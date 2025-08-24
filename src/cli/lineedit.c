/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <types.h>

#include "ctype.h"

#include "cli_history.h"
#include "cli_termesc.h"

typedef struct cmdline_struct {
	char buf[MSH_CMDLINE_CHAR_MAX];
	int pos;	 /* cursor position (start at 1 orignin, 0 means empty line) */
	int linelen; /* length of input char of line EXCLUDING trailing null */
	/* The buffer used for Cut&Paste */
	char clipboard[MSH_CMDLINE_CHAR_MAX];
} cmdline_t;

static cmdline_t CmdLine;
static int bCmdLineInitialized;

static void cmdline_clear(cmdline_t *pcmdline) {
	memset(pcmdline->buf, '\0', MSH_CMDLINE_CHAR_MAX);
	pcmdline->pos = 0;
	pcmdline->linelen = 0;
}

static void cmdline_init(cmdline_t *pcmdline) {
	cmdline_clear(pcmdline);
	memset(pcmdline->clipboard, '\0', MSH_CMDLINE_CHAR_MAX);
}

static char *prompt_string = MSH_CMD_PROMPT;

void msh_set_prompt(char *str) {
	prompt_string = str;
}

static void cmdline_kill(cmdline_t *pcmdline) {
	int i;
	for (i = 0; i < pcmdline->pos; i++) { uart_putchar('\b'); }
	for (i = 0; i < pcmdline->linelen; i++) { uart_putchar(' '); }
	for (i = 0; i < pcmdline->linelen; i++) { uart_putchar('\b'); }
	cmdline_clear(pcmdline);
}

static void cmdline_set(cmdline_t *pcmdline, const char *str) {
	int len;

	cmdline_kill(pcmdline);
	len = strlen(str);
	strcpy(pcmdline->buf, str);
	uart_puts(str);
	pcmdline->pos = len;
	pcmdline->linelen = len;
}

static int cmdline_insert_char(cmdline_t *pcmdline, unsigned char c) {
	/* Check if the line buffer can hold another one char */
	if (pcmdline->linelen >= MSH_CMDLINE_CHAR_MAX - 1) {
		/* buffer is full */
		uart_putchar('\a');
		return 0;
	}

	uart_putchar(c);
	/* Is cursor at the end of the cmdline ? */
	if (pcmdline->pos == pcmdline->linelen) {
		/* just append */
		pcmdline->buf[pcmdline->pos] = c;
	} else {
		/* slide the strings after the cursor to the right */
		int i;
		uart_puts(&pcmdline->buf[pcmdline->pos]);
		for (i = pcmdline->linelen; i > pcmdline->pos; i--) {
			pcmdline->buf[i] = pcmdline->buf[i - 1];
			uart_putchar('\b');
		}
		pcmdline->buf[pcmdline->pos] = c;
	}
	pcmdline->pos++;
	pcmdline->linelen++;
	pcmdline->buf[pcmdline->linelen] = '\0'; /* just for safty */
	return 1;
}

static int cmdline_backspace(cmdline_t *pcmdline) {
	if (pcmdline->pos <= 0) {
		uart_putchar('\a');
		return 0;
	}
	uart_putchar('\b');
	/* Is cursor at the end of the cmdline ? */
	if (pcmdline->pos == pcmdline->linelen) {
		uart_putchar(' ');
		uart_putchar('\b');
	} else {
		int i;
		/* slide the characters after cursor position to the left */
		for (i = pcmdline->pos; i < pcmdline->linelen; i++) {
			pcmdline->buf[i - 1] = pcmdline->buf[i];
			uart_putchar(pcmdline->buf[i]);
		}
		uart_putchar(' ');
		/* put the cursor to its orignlal position */
		/* +1 in for () is for uart_putchar(' ') in the previous line */
		for (i = pcmdline->pos; i < pcmdline->linelen + 1; i++) { uart_putchar('\b'); }
	}
	pcmdline->buf[pcmdline->linelen - 1] = '\0';
	pcmdline->pos--;
	pcmdline->linelen--;
	return 1;
}

static int cmdline_delete(cmdline_t *pcmdline) {
	if (pcmdline->linelen <= pcmdline->pos) {
		/* No more charactors to delete.
         * i.e, cursor is the rightmost pos of the line.  */
		uart_putchar('\a');
		return 0;
	} else {
		int i;
		/* slide the chars on and after cursor position to the left */
		for (i = pcmdline->pos; i < pcmdline->linelen - 1; i++) {
			pcmdline->buf[i] = pcmdline->buf[i + 1];
			uart_putchar(pcmdline->buf[i]);
		}
		uart_putchar(' ');
		/* put the cursor to its orignlal position */
		for (i = pcmdline->pos; i < pcmdline->linelen; i++) { uart_putchar('\b'); }
	}
	pcmdline->buf[pcmdline->linelen - 1] = '\0';
	pcmdline->linelen--;
	return 1;
}

static int cmdline_cursor_left(cmdline_t *pcmdline) {
	if (pcmdline->pos > 0) {
		uart_putchar('\b');
		pcmdline->pos--;
		return 1;
	} else {
		uart_putchar('\a');
		return 0;
	}
}

static int cmdline_cursor_right(cmdline_t *pcmdline) {
	if (pcmdline->pos < pcmdline->linelen) {
		uart_putchar(pcmdline->buf[pcmdline->pos++]);
		return 1;
	} else {
		uart_putchar('\a');
		return 0;
	}
}

static void cmdline_cursor_linehead(cmdline_t *pcmdline) {
	while (pcmdline->pos > 0) {
		uart_putchar('\b');
		pcmdline->pos--;
	}
}

static void cmdline_cursor_linetail(cmdline_t *pcmdline) {
	while (pcmdline->pos < pcmdline->linelen) { uart_putchar(pcmdline->buf[pcmdline->pos++]); }
}

static void cmdline_yank(cmdline_t *pcmdline) {
	if (strlen(pcmdline->clipboard) == 0) {
		/* no string in the clipboard */
		uart_putchar('\a');
	} else {
		int i = 0;
		while (pcmdline->clipboard[i] != '\0' && cmdline_insert_char(pcmdline, pcmdline->clipboard[i])) { i++; }
	}
}

static void cmdline_killtail(cmdline_t *pcmdline) {
	int i;
	if (pcmdline->pos == pcmdline->linelen) {
		/* nothing to kill */
		uart_putchar('\a');
	}

	/* copy chars on and right of the cursor to the clipboar */
	strcpy(pcmdline->clipboard, &pcmdline->buf[pcmdline->pos]);

	/* erase chars on and right of the cursor on terminal */
	for (i = pcmdline->pos; i < pcmdline->linelen; i++) { uart_putchar(' '); }
	for (i = pcmdline->pos; i < pcmdline->linelen; i++) { uart_putchar('\b'); }

	/* erase chars on and right of the cursor in buf */
	pcmdline->buf[pcmdline->pos] = '\0';
	pcmdline->linelen = pcmdline->pos;
}

static void cmdline_killword(cmdline_t *pcmdline) {
	int i, j;
	if (pcmdline->pos == 0) {
		uart_putchar('\a');
		return;
	}
	/* search backward for a word to kill */
	i = 0;
	while (i < pcmdline->pos && pcmdline->buf[pcmdline->pos - i - 1] == ' ') { i++; }
	while (i < pcmdline->pos && pcmdline->buf[pcmdline->pos - i - 1] != ' ') { i++; }

	/* copy the word to clipboard */
	j = 0;
	while (j < i) {
		pcmdline->clipboard[j] = pcmdline->buf[pcmdline->pos - i + j];
		j++;
	}
	pcmdline->clipboard[j] = '\0';

	/* kill the word */
	j = 0;
	while (j < i) {
		cmdline_backspace(pcmdline);
		j++;
	}
}

static int histnum;

char curline[MSH_CMDLINE_CHAR_MAX];

const char *histline;

static int cursor_inputchar(cmdline_t *pcmdline, unsigned char c) {
	unsigned char input = c;
	if (input == '\033') {
		char second, third;
		second = uart_getchar();
		third = uart_getchar();
		if (second == '[') {
			switch (third) {
				case 'A':
					input = MSH_KEYBIND_HISTPREV;
					break;
				case 'B':
					input = MSH_KEYBIND_HISTNEXT;
					break;
				case 'C':
					input = MSH_KEYBIND_CURRIGHT;
					break;
				case 'D':
					input = MSH_KEYBIND_CURLEFT;
					break;
				default:;
					/* do nothing */
			}
		} else {
			/* do nothing */
		}
	}

	switch (input) {
		/*
         * End of input if newline char.
         */
		case MSH_KEYBIND_ENTER:
			uart_putchar('\n');
			return 0;

		case '\t':
			/* tab sould be comverted to a space */
			cmdline_insert_char(pcmdline, ' ');
			break;

		case MSH_KEYBIND_DISCARD:
			cmdline_clear(pcmdline);
			uart_putchar('\n');
			return 0;

		case MSH_KEYBIND_BACKSPACE:
			cmdline_backspace(pcmdline);
			break;

		case MSH_KEYBIND_DELETE:
		case 0x7F: /* ASCII DEL.  Should be used as BS ?*/
			cmdline_delete(pcmdline);
			break;

		case MSH_KEYBIND_KILLLINE:
			cmdline_kill(pcmdline);
			break;

		case MSH_KEYBIND_CLEAR:
			cmdline_cursor_linehead(pcmdline);
			uart_puts(TERMESC_CLEAR);
			uart_puts(prompt_string);
			cmdline_cursor_linetail(pcmdline);
			break;

		case MSH_KEYBIND_CURLEFT:
			cmdline_cursor_left(pcmdline);
			break;

		case MSH_KEYBIND_CURRIGHT:
			cmdline_cursor_right(pcmdline);
			break;

		case MSH_KEYBIND_LINEHEAD:
			cmdline_cursor_linehead(pcmdline);
			break;

		case MSH_KEYBIND_LINETAIL:
			cmdline_cursor_linetail(pcmdline);
			break;

		case MSH_KEYBIND_YANK:
			cmdline_yank(pcmdline);
			break;

		case MSH_KEYBIND_KILLTAIL:
			cmdline_killtail(pcmdline);
			break;

		case MSH_KEYBIND_KILLWORD:
			cmdline_killword(pcmdline);
			break;

		case MSH_KEYBIND_HISTPREV:
			if (histnum == 0) {
				/* save current line before overwrite with history */
				strcpy(curline, pcmdline->buf);
			}
			histline = history_get(histnum);
			if (histline != NULL) {
				cmdline_set(pcmdline, histline);
				histnum++;
			} else {
				uart_putchar('\a');
			}
			break;

		case MSH_KEYBIND_HISTNEXT:
			if (histnum == 1) {
				histnum = 0;
				cmdline_set(pcmdline, curline);
			} else if (histnum > 1) {
				histline = history_get(histnum - 2);
				if (histline != NULL) {
					cmdline_set(pcmdline, histline);
					histnum--;
				} else {
					uart_putchar('\a'); /* no newer hist */
				}
			} else {
				uart_putchar('\a'); /* invalid (negative) histnum value */
			}

			break;

		default:
			if (isprint(c)) {
				if (pcmdline->pos < MSH_CMDLINE_CHAR_MAX - 1) {
					cmdline_insert_char(pcmdline, c);
				}
			}
			break;
	}

	return 1 /*true*/;
}


int msh_get_cmdline(char *linebuf) {
	if (!bCmdLineInitialized) {
		cmdline_init(&CmdLine);
		bCmdLineInitialized = 1; /* true */
	} else {
		cmdline_clear(&CmdLine);
	}
	uart_puts(prompt_string);

	while (cursor_inputchar(&CmdLine, uart_getchar()))
		;

	history_append(CmdLine.buf);
	histnum = 0; /* reset active histnum */

	strcpy(linebuf, CmdLine.buf);
	return (strlen(CmdLine.buf));
}
