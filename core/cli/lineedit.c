/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file lineedit.c
 * @brief UART line editor with history and terminal escape-key handling.
 *
 * The editor keeps a fixed-size command buffer, redraws only the affected
 * terminal span, and translates common ANSI cursor sequences into shell key
 * bindings. The command state can be heap-backed for applications that
 * initialize the allocator before entering the shell.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <types.h>

#include "ctype.h"

#include <cli/cli_history.h>
#include <cli/cli_termesc.h>
#ifdef CONFIG_CLI_HEAP_STORAGE
#include <malloc.h>
#endif

/** @brief Mutable command line, cursor, and cut/paste state. */
typedef struct cmdline_struct {
	char buf[MSH_CMDLINE_CHAR_MAX];
	int pos; /* cursor position (start at 1 orignin, 0 means empty line) */
	int linelen; /* length of input char of line EXCLUDING trailing null */
	/* The buffer used for Cut&Paste */
	char clipboard[MSH_CMDLINE_CHAR_MAX];
	/* The line saved while browsing command history. */
	char curline[MSH_CMDLINE_CHAR_MAX];
} cmdline_t;

#ifdef CONFIG_CLI_HEAP_STORAGE
static cmdline_t *cmdline;
#define CmdLine (*cmdline)
#else
static cmdline_t CmdLine;
#endif
static int bCmdLineInitialized;

/**
 * @brief Clear text and reset the cursor to an empty command line.
 * @param[in,out] pcmdline Editor state to clear.
 */
static void cmdline_clear(cmdline_t *pcmdline)
{
	memset(pcmdline->buf, '\0', MSH_CMDLINE_CHAR_MAX);
	pcmdline->pos = 0;
	pcmdline->linelen = 0;
}

/**
 * @brief Clear a command line and its cut/paste clipboard.
 * @param[in,out] pcmdline Editor state to initialize.
 */
static void cmdline_init(cmdline_t *pcmdline)
{
	cmdline_clear(pcmdline);
	memset(pcmdline->clipboard, '\0', MSH_CMDLINE_CHAR_MAX);
	memset(pcmdline->curline, '\0', MSH_CMDLINE_CHAR_MAX);
}

static char *prompt_string = MSH_CMD_PROMPT;

/**
 * @brief Select the prompt displayed before each command line.
 * @param[in] str NUL-terminated prompt string retained by the editor.
 */
void msh_set_prompt(char *str)
{
	prompt_string = str;
}

/**
 * @brief Erase the complete visible line and reset its buffer.
 * @param[in,out] pcmdline Editor state and terminal row to clear.
 */
static void cmdline_kill(cmdline_t *pcmdline)
{
	int i;
	for (i = 0; i < pcmdline->pos; i++) {
		uart_putchar('\b');
	}
	for (i = 0; i < pcmdline->linelen; i++) {
		uart_putchar(' ');
	}
	for (i = 0; i < pcmdline->linelen; i++) {
		uart_putchar('\b');
	}
	cmdline_clear(pcmdline);
}

/**
 * @brief Replace the editable line with a new string and redraw it.
 * @param[in,out] pcmdline Editor state to update.
 * @param[in] str Replacement text; it must fit the fixed command buffer.
 */
static void cmdline_set(cmdline_t *pcmdline, const char *str)
{
	int len;

	cmdline_kill(pcmdline);
	len = strlen(str);
	strcpy(pcmdline->buf, str);
	uart_puts(str);
	pcmdline->pos = len;
	pcmdline->linelen = len;
}

/**
 * @brief Insert one character at the current cursor position.
 * @param[in,out] pcmdline Editor state to update and redraw.
 * @param[in] c Character to insert.
 * @return One when inserted, zero when the fixed buffer is full.
 */
static int cmdline_insert_char(cmdline_t *pcmdline, unsigned char c)
{
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

/**
 * @brief Delete the character immediately before the cursor.
 * @param[in,out] pcmdline Editor state and terminal line to update.
 * @return One when a character was removed, zero at the line start.
 */
static int cmdline_backspace(cmdline_t *pcmdline)
{
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
		for (i = pcmdline->pos; i < pcmdline->linelen + 1; i++) {
			uart_putchar('\b');
		}
	}
	pcmdline->buf[pcmdline->linelen - 1] = '\0';
	pcmdline->pos--;
	pcmdline->linelen--;
	return 1;
}

/**
 * @brief Delete the character under the cursor.
 * @param[in,out] pcmdline Editor state and terminal line to update.
 * @return One when a character was removed, zero at the line end.
 */
static int cmdline_delete(cmdline_t *pcmdline)
{
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
		for (i = pcmdline->pos; i < pcmdline->linelen; i++) {
			uart_putchar('\b');
		}
	}
	pcmdline->buf[pcmdline->linelen - 1] = '\0';
	pcmdline->linelen--;
	return 1;
}

/**
 * @brief Move the cursor left when it is not already at column zero.
 * @param[in,out] pcmdline Editor state and terminal cursor to update.
 * @return One when moved, zero when already at the line start.
 */
static int cmdline_cursor_left(cmdline_t *pcmdline)
{
	if (pcmdline->pos > 0) {
		uart_putchar('\b');
		pcmdline->pos--;
		return 1;
	} else {
		uart_putchar('\a');
		return 0;
	}
}

/**
 * @brief Move the cursor right when it is not already at line end.
 * @param[in,out] pcmdline Editor state and terminal cursor to update.
 * @return One when moved, zero when already at the line end.
 */
static int cmdline_cursor_right(cmdline_t *pcmdline)
{
	if (pcmdline->pos < pcmdline->linelen) {
		uart_putchar(pcmdline->buf[pcmdline->pos++]);
		return 1;
	} else {
		uart_putchar('\a');
		return 0;
	}
}

/**
 * @brief Move the cursor to the beginning of the editable line.
 * @param[in,out] pcmdline Editor state and terminal cursor to update.
 */
static void cmdline_cursor_linehead(cmdline_t *pcmdline)
{
	while (pcmdline->pos > 0) {
		uart_putchar('\b');
		pcmdline->pos--;
	}
}

/**
 * @brief Move the cursor to the end of the editable line.
 * @param[in,out] pcmdline Editor state and terminal cursor to update.
 */
static void cmdline_cursor_linetail(cmdline_t *pcmdline)
{
	while (pcmdline->pos < pcmdline->linelen) {
		uart_putchar(pcmdline->buf[pcmdline->pos++]);
	}
}

/**
 * @brief Insert the clipboard contents at the current cursor position.
 * @param[in,out] pcmdline Editor state and terminal row to update.
 */
static void cmdline_yank(cmdline_t *pcmdline)
{
	if (strlen(pcmdline->clipboard) == 0) {
		/* no string in the clipboard */
		uart_putchar('\a');
	} else {
		int i = 0;
		while (pcmdline->clipboard[i] != '\0' && cmdline_insert_char(pcmdline, pcmdline->clipboard[i])) {
			i++;
		}
	}
}

/**
 * @brief Cut text from the cursor through line end into the clipboard.
 * @param[in,out] pcmdline Editor state, clipboard, and terminal row to update.
 */
static void cmdline_killtail(cmdline_t *pcmdline)
{
	int i;
	if (pcmdline->pos == pcmdline->linelen) {
		/* nothing to kill */
		uart_putchar('\a');
	}

	/* copy chars on and right of the cursor to the clipboar */
	strcpy(pcmdline->clipboard, &pcmdline->buf[pcmdline->pos]);

	/* erase chars on and right of the cursor on terminal */
	for (i = pcmdline->pos; i < pcmdline->linelen; i++) {
		uart_putchar(' ');
	}
	for (i = pcmdline->pos; i < pcmdline->linelen; i++) {
		uart_putchar('\b');
	}

	/* erase chars on and right of the cursor in buf */
	pcmdline->buf[pcmdline->pos] = '\0';
	pcmdline->linelen = pcmdline->pos;
}

/**
 * @brief Cut the previous whitespace-delimited word into the clipboard.
 * @param[in,out] pcmdline Editor state, clipboard, and terminal row to update.
 */
static void cmdline_killword(cmdline_t *pcmdline)
{
	int i, j;
	if (pcmdline->pos == 0) {
		uart_putchar('\a');
		return;
	}
	/* search backward for a word to kill */
	i = 0;
	while (i < pcmdline->pos && pcmdline->buf[pcmdline->pos - i - 1] == ' ') {
		i++;
	}
	while (i < pcmdline->pos && pcmdline->buf[pcmdline->pos - i - 1] != ' ') {
		i++;
	}

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

const char *histline;

/**
 * @brief Apply one input byte or escape sequence to the editor.
 * @param[in,out] pcmdline Editor state to update.
 * @param[in] c Input byte received from the UART.
 * @return One while editing should continue, zero after Enter or discard.
 */
static int cursor_inputchar(cmdline_t *pcmdline, unsigned char c)
{
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
		} else if (second == 'O' && third == 'M') {
			/* VT100 application keypad Enter. */
			input = MSH_KEYBIND_ENTER;
		} else {
			/* do nothing */
		}
	}

	switch (input) {
	/*
         * End of input if newline char.
         */
	case MSH_KEYBIND_ENTER:
	case '\r':
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
			strcpy(pcmdline->curline, pcmdline->buf);
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
			cmdline_set(pcmdline, pcmdline->curline);
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

/**
 * @brief Read, edit, and return one command line from the UART.
 * @param[out] linebuf Destination buffer receiving the NUL-terminated line.
 * @return Number of characters in the returned line, excluding its terminator.
 */
int msh_get_cmdline(char *linebuf)
{
#ifdef CONFIG_CLI_HEAP_STORAGE
	if (!cmdline)
		cmdline = malloc(sizeof(*cmdline));
	if (!cmdline)
		return 0;
#endif
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
