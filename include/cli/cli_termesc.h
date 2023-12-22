/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __CLI_TERMESC_H__
#define __CLI_TERMESC_H__

/*
 *  Terminal ESC sequences
 */
#define TERMESC_CLEAR       "\033[2J\033[0;0H"
#define TERMESC_RESET       "\033c"

/* FONT STYLE */
#define TERMESC_FONT_NORMAL "\033[00m"
#define TERMESC_FONT_BOLD   "\033[00m"
#define TERMESC_FONT_ULINE  "\033[04m"

/* FONT COLOR */
#define TERMESC_COL_BLACK   "\033[30m"
#define TERMESC_COL_RED     "\033[31m"
#define TERMESC_COL_GREEN   "\033[32m"
#define TERMESC_COL_YELLOW  "\033[33m"
#define TERMESC_COL_BLUE    "\033[34m"
#define TERMESC_COL_MAGENTA "\033[35m"
#define TERMESC_COL_CYAN    "\033[36m"
#define TERMESC_COL_WHITE   "\033[37m"

/* BACKGROUND COLOR OF FONTS */
#define TERMESC_BACK_BLACK   "\033[40m"
#define TERMESC_BACK_RED     "\033[41m"
#define TERMESC_BACK_GREEN   "\033[42m"
#define TERMESC_BACK_YELLOW  "\033[43m"
#define TERMESC_BACK_BLUE    "\033[44m"
#define TERMESC_BACK_MAGENTA "\033[45m"
#define TERMESC_BACK_CYAN    "\033[46m"
#define TERMESC_BACK_WHITE   "\033[47m"


#endif/*__CLI_TERMESC_H__*/
