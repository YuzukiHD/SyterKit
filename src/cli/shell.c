#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <types.h>

#include <log.h>

#include "cli.h"
#include "cli_history.h"
#include "cli_termesc.h"

const msh_command_entry *msh_user_commands;

const msh_command_entry empty_commands[] = {
		msh_command_end,
};

int syterkit_shell_attach(const msh_command_entry *cmdlist) {
	char linebuf[MSH_CMDLINE_CHAR_MAX];

	/* Loop for reading line (forever). */
	int argc;
	char *argv[MSH_CMDARGS_MAX];
	char argbuf[MSH_CMDLINE_CHAR_MAX];

	/* set msh_user_commands */
	if (cmdlist != NULL) {
		msh_user_commands = cmdlist;
	} else {
		/* avoid read NULL ptr */
		msh_user_commands = empty_commands;
	}

	msh_set_prompt("SyterKit> ");

	while (1) {
		/* Read a input line. */
		if (msh_get_cmdline(linebuf) == 0) {
			/* got empty input */
			continue;
		}

		/* Loop for parse line and executing commands. */
		const char *linebufp = linebuf;
		while (1) {
			const char *ret_parse;
			int ret_command = -1;

			ret_parse = msh_parse_line(linebufp, argbuf, &argc, argv);

			if (ret_parse == NULL) {
				uart_puts("Syntax error\n");
				break; /* discard this line */
			}

			if (strlen(argv[0]) <= 0) {
				break; /* empty input line */
			}

#if (MSH_ARGS_DEBUG == 1)
			uart_puts(" args> ");
			for (int i = argc - 1; i > 0; i--) {
				uart_puts(argv[i]);
				if (i > 1) {
					uart_puts(", ");
				}
			}
			uart_puts("\n");
#endif
			ret_command = msh_do_command(msh_user_commands, argc, (const char **) argv);

			if (ret_command < 0) {
				ret_command = msh_do_command(msh_builtin_commands, argc, (const char **) argv);
			}

			if (ret_command < 0) {
				uart_puts("command not found: \'");
				uart_puts(argv[0]);
				uart_puts("'\n");
			}

			/*
             * Do we have more sentents remained in linebuf
             * separated by a ';' or a '\n' ?
             */
			if (ret_parse == linebufp) {
				/* No, we don't */
				break;
			} else {
				/* Yes, we have. We have to parse rest of lines, which begins with char* ret_parse; */
				linebufp = ret_parse;
			}
		}
	}
	return 0;
}
