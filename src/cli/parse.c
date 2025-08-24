/* SPDX-License-Identifier: GPL-2.0+ */

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "cli_config.h"

typedef struct parse_state_struct {
	const char *readpos;
	char *writepos;
} parse_state_t;

/*
 * E<0 : Syntax error type E (FIXME: no error types defined)
 *   0 : No characters to read (';' or '\0')
 * N>1 : read N chars
 *
 * If error is returned, state of parse_state_t may not be consistent
 * and should never re-used. (whole input line should be discarded.)
 */
static int read_token(parse_state_t *pstate) {
	int readcount = 0; /* FIXME counting is not correct */
	bool is_squoted = false;
	bool is_dquoted = false;

	while (1) {
		char ch = *pstate->readpos;

		if (ch == '\0') {
			break; /* end of input */
		}
		readcount++;

		/*  We are in quote */
		if (is_squoted || is_dquoted) {
			if ((is_squoted && ch == '\'') || (is_dquoted && ch == '"')) {
				/*
                 * FIXME : For now, we treat dquote as squote.
                 */
				/* Closing quote */
				is_squoted = false;
				is_dquoted = false;
			} else {
				/* Read the char as it is. */
				*pstate->writepos++ = ch;
			}
		}
		/*
         * We are NOT in quote
         */
		else {
			/* quote */
			if (ch == '\'') {
				is_squoted = true;
			} else if (ch == '"') {
				is_dquoted = true;
			}

			/* Backslash (Escaping): read a char ahead */
			else if (ch == MSH_CMD_ESCAPE_CHAR) {
				/* read a char ahead */
				pstate->readpos++;
				ch = *pstate->readpos;
				if (!isprint((unsigned) ch)) {
					/* You can only escape normal chars */
					return -1;
				} else {
					readcount++;
					*pstate->writepos++ = ch;
				}
			}

			/* A blank not in quote or not escaped, or a
             * command line separator (';') makes this argument done. */
			else if (isspace((unsigned) ch) || ch == MSH_CMD_SEP_CHAR) {
				readcount--;
				break; /* end of current argument */
			}

			/* Normal chars */
			else if (isprint((unsigned) ch)) {
				*pstate->writepos++ = ch;
			}

			/* NON Printable (control code) char  */
			else {
				/*
                 * If you want to just Ignore invalid characters,
                 * comment-out the blow.
                 */
				return -1; /* Syntax error */
			}
		}
		pstate->readpos++;
	}

	/*
     * Finally, check if state is consistent,
     * I.e., check for no-closing quote etc.
     */
	if (is_squoted || is_dquoted) {
		return -1; /* Syntax error */
	} else {
		*pstate->writepos++ = '\0';
		return readcount;
	}
}

const char *msh_parse_line(const char *cmdline, char *argvbuf, int *pargc, char **argv) {
	/*
     * Prepare and initialize a parse_state_t.
     */
	parse_state_t state;
	state.readpos = cmdline;
	state.writepos = argvbuf;

	*pargc = 0;
	argv[0] = argvbuf;

	while (*state.readpos != '\0') {

		/*
         * Skip preceeding spaces or ';'
         */
		while (isspace((unsigned) *state.readpos)) { state.readpos++; }

		int ret = read_token(&state);

		/*
         * Syntax error
         */
		if (ret < 0) {
			return NULL;
		} else
			/*
         * No more chars to read. Case I
         */
			if (ret == 0) {
				switch (*(state.readpos)) {
					case '\0':
						return cmdline;
					case MSH_CMD_SEP_CHAR:
						return (state.readpos + 1);
					default:
						uart_puts("Fatal error in parse() \n");
						return NULL;
				}
			}

			/*
         * Normal
         */
			else {
				(*pargc)++;
				char stopchar = *(state.readpos);
				/*
             * No more chars to read. Case II
             */
				if (stopchar == '\0') {
					return cmdline;
				}

				/*
             * End of command by ';'.
             * Tell the caller where to restart.
             */
				if (stopchar == MSH_CMD_SEP_CHAR) {
					state.readpos++; /* skip ';' */
					return (state.readpos);
				}

				/*
             * read_token() stoped by a argument separator.
             */
				else if (isspace((unsigned) stopchar)) {
					argv[*pargc] = state.writepos;
					continue;
				}

				/*
             * Can't be!!
             */
				else {
					uart_puts("Fatal error in parse() ");
					return NULL;
				}
			}
	}

	/* Tell the caller that whole line has processed */
	return cmdline;
}
