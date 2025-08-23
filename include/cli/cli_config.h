/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __CLI_CONFIG_H__
#define __CLI_CONFIG_H__

#include <uart.h>

/* ************************************************************************* *
 *     Buffer Sizes for Commandline Editting
 * ************************************************************************* */
/* maximum chars per line, INCLUDEING a traling null char */
#define MSH_CMDLINE_CHAR_MAX (300)

/* maximum argument a commandline can hold, i.e., max of argc value */
#define MSH_CMDARGS_MAX (16)

/* maximum number of history */
#define MSH_CMD_HISTORY_MAX (8)

/* default prompt string. (cmdedit.c) */
#define MSH_CMD_PROMPT "SyterKit> "

/* enable args debug */
#define MSH_ARGS_DEBUG (0)

/* ************************************************************************* *
 *     Keybinds etc
 * ************************************************************************* */
/* keybinds */
#define MSH_CTRL_KEY(c) (~0x60 & c)
/*  the default is emacs flavor
   Ctrl-H  Backspace
   Ctrl-C  Discard line

   Ctrl-F  Cursor right
   Ctrl-B  Cursor left
   Ctrl-A  Line head
   Ctrl-E  Line tail
   Ctrl-D  Delete
   Ctrl-U  Kill line

   Ctrl-P  Previous history
   Ctrl-N  Next history
   Ctrl-L  Clear screen
 */
#define MSH_CTRL_KEY_DECODE(c) (0x60 | c)
#define MSH_KEYBIND_ENTER '\n'
#define MSH_KEYBIND_BACKSPACE MSH_CTRL_KEY('h')
#define MSH_KEYBIND_DISCARD MSH_CTRL_KEY('c')
#define MSH_KEYBIND_DELETE MSH_CTRL_KEY('d')
#define MSH_KEYBIND_KILLLINE MSH_CTRL_KEY('u')
#define MSH_KEYBIND_CURRIGHT MSH_CTRL_KEY('f')
#define MSH_KEYBIND_CURLEFT MSH_CTRL_KEY('b')
#define MSH_KEYBIND_LINEHEAD MSH_CTRL_KEY('a')
#define MSH_KEYBIND_LINETAIL MSH_CTRL_KEY('e')
#define MSH_KEYBIND_YANK MSH_CTRL_KEY('y')
#define MSH_KEYBIND_KILLTAIL MSH_CTRL_KEY('k')
#define MSH_KEYBIND_KILLWORD MSH_CTRL_KEY('w')
#define MSH_KEYBIND_CLEAR MSH_CTRL_KEY('l')
#define MSH_KEYBIND_HISTPREV MSH_CTRL_KEY('p')
#define MSH_KEYBIND_HISTNEXT MSH_CTRL_KEY('n')

/* parse.c */
#define MSH_CMD_DQUOTE_CHAR '"'	 /* double quote */
#define MSH_CMD_SQUOTE_CHAR '\'' /* single quote */
#define MSH_CMD_ESCAPE_CHAR '\\' /* backslash */
#define MSH_CMD_SEP_CHAR ';'	 /* command separator */
#define MSH_CMD_FS_CHAR ' '		 /* field separator */


#endif /*__CLI_CONFIG_H__*/
