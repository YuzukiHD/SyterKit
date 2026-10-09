/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file screenfetch.c
 * @brief Shell command `screenfetch`: logo and system information, like fastfetch.
 *
 * The logo is ASCII art in 24 bit colors. The text lines are collected first so the logo and the information can
 * be printed side by side.
 */
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <types.h>

#include <config.h>
#include <format.h>
#include <log.h>
#include <timer.h>

#include <cli/cli.h>
#include <screenfetch.h>
#ifdef CONFIG_VT
#include <vt.h>
#endif

msh_define_help(screenfetch, "show the logo and system information",
	"Usage: screenfetch [--no-logo]\n"
	"    Prints the SyterKit logo and information about the build and the board.\n"
	"    --no-logo, -n  print the information only.\n");

/*
 * The logo is the name in the same letters as the boot banner, one color per row. The bytes 1..5 in the text
 * switch the color to that entry of the palette; a row ends at '\n' and is padded to FETCH_LOGO_COLS when printed.
 */
#define FETCH_LOGO_COLS 36
#define FETCH_LOGO_ROWS 5

#define LOGO_ROW1 "\001"
#define LOGO_ROW2 "\002"
#define LOGO_ROW3 "\003"
#define LOGO_ROW4 "\004"
#define LOGO_ROW5 "\005"

/* the 256 color palette entries 45, 39, 33, 27 and 21 */
static const unsigned int fetch_logo_palette[5] = {
	0x00d7ff, 0x00afff, 0x0087ff, 0x005fff, 0x0000ff,
};

static const char fetch_logo_text[] =
	LOGO_ROW1 " _____     _           _____ _ _   " "\n"
	LOGO_ROW2 "|   __|_ _| |_ ___ ___|  |  |_| |_ " "\n"
	LOGO_ROW3 "|__   | | |  _| -_|  _|    -| | _| " "\n"
	LOGO_ROW4 "|_____|_  |_| |___|_| |__|__|_|_|  " "\n"
	LOGO_ROW5 "      |___|                        " "\n";

#define LOGO_ROWS FETCH_LOGO_ROWS
#define LOGO_GAP  "   "
#define MAX_LINES 16

struct fetch_lines {
	char text[MAX_LINES][80];
	unsigned int count;
};

/* the lines live on the stack of screenfetch(): bss above 0x2b000 can belong to the display engine */
static struct fetch_lines *lines;

void __attribute__((weak)) board_fetch_info(screenfetch_info_t *info)
{
	(void)info;
}

struct sink {
	char *buf;
	unsigned int len, cap;
};

static void sink_putc(void *arg, char c)
{
	struct sink *s = arg;

	if (s->len + 1 < s->cap)
		s->buf[s->len++] = c;
}

static void sink_str(struct sink *s, const char *str)
{
	for (; *str; str++)
		sink_putc(s, *str);
}

static void add(const char *key, const char *fmt, ...)
{
	struct sink s;
	va_list ap;

	if (lines->count >= MAX_LINES)
		return;
	s.buf = lines->text[lines->count];
	s.len = 0;
	s.cap = sizeof(lines->text[0]);
	if (key) {
		/* bold cyan key, reset, then the value */
		sink_str(&s, "\033[1;36m");
		sink_str(&s, key);
		sink_str(&s, "\033[0m: ");
	}
	va_start(ap, fmt);
	vformat(sink_putc, &s, fmt, ap);
	va_end(ap);
	s.buf[s.len] = '\0';
	lines->count++;
}

static void add_title(void)
{
	static const char user[] = PROJECT_NAME, host[] = "bootloader";
	char under[sizeof(user) + sizeof(host)];
	unsigned int i;

	add(NULL, "\033[1;36m%s\033[0m@\033[1;36m%s\033[0m", user, host);
	for (i = 0; i < sizeof(user) - 1 + 1 + sizeof(host) - 1; i++)
		under[i] = '-';
	under[i] = '\0';
	add(NULL, "%s", under);
}

/* The SGR state is tracked so only changes are sent: the UART is slow. */
#define COLOR_NONE 0xffffffffU

/* Prints one row of the logo, padded to its width, and returns the start of the next row. */
static const char *put_logo_row(const char *p)
{
	unsigned int cols = 0;

	for (; *p != '\0' && *p != '\n'; p++) {
		unsigned int c = (unsigned char)*p;

		if (c >= 1U && c <= sizeof(fetch_logo_palette) / sizeof(fetch_logo_palette[0])) {
			unsigned int rgb = fetch_logo_palette[c - 1U];

			printf("\033[38;2;%u;%u;%um", (unsigned int)(rgb >> 16 & 0xff), (unsigned int)(rgb >> 8 & 0xff),
			       (unsigned int)(rgb & 0xff));
		} else {
			printf("%c", *p);
			cols++;
		}
	}
	printf("\033[0m%*s", (int)(FETCH_LOGO_COLS - cols), "");
	return *p == '\n' ? p + 1 : p;
}

static void put_palette_row(int first)
{
	int c;

	for (c = first; c < first + 8; c++)
		printf("\033[%dm   ", c);
	printf("\033[0m");
}

void screenfetch(bool show_logo)
{
	struct fetch_lines storage;
	screenfetch_info_t info = { 0 };
	uint32_t up = time_us() - get_init_timestamp();
	const char *logo_row = fetch_logo_text;
	unsigned int i, rows, info_rows, logo_top, info_top;

	board_fetch_info(&info);
	lines = &storage;
	lines->count = 0;

	add_title();
	add("OS", "%s v%s", PROJECT_NAME, PROJECT_VERSION);
	if (info.model)
		add("Host", "%s", info.model);
	add("Commit", "%s", PROJECT_GIT_HASH);
	add("Compiler", "%s %s", PROJECT_C_COMPILER, PROJECT_C_COMPILER_VERSION);
#if __riscv_xlen == 32
	add("Arch", "riscv32");
#elif __riscv_xlen == 64
	add("Arch", "riscv64");
#elif defined(__aarch64__)
	add("Arch", "aarch64");
#elif defined(__arm__)
	add("Arch", "arm");
#endif
	if (info.soc)
		add("SoC", "%s", info.soc);
	if (info.cpu)
		add("CPU", "%s", info.cpu);
	if (info.mem_kb)
		add("Memory", "%u MiB", (unsigned int)(info.mem_kb / 1024U));
	if (info.has_sid)
		add("Chip ID", "%08x%08x%08x%08x", (unsigned int)info.sid[0], (unsigned int)info.sid[1],
		    (unsigned int)info.sid[2], (unsigned int)info.sid[3]);
	add("Uptime", "%u.%03u s", (unsigned int)(up / 1000000U), (unsigned int)(up % 1000000U / 1000U));
#ifdef CONFIG_VT
	if (vt_is_ready())
		add("Terminal", "vt %ux%u", (unsigned int)vt_cols(), (unsigned int)vt_rows());
#endif

	/* the information lines, a blank line and the two palette rows */
	info_rows = lines->count + 3U;
	rows = show_logo && LOGO_ROWS > info_rows ? LOGO_ROWS : info_rows;
	logo_top = (rows - LOGO_ROWS) / 2U;
	info_top = (rows - info_rows) / 2U;

	printf("\n");
	for (i = 0; i < rows; i++) {
		if (show_logo) {
			if (i >= logo_top && i < logo_top + LOGO_ROWS)
				logo_row = put_logo_row(logo_row);
			else
				printf("%*s", FETCH_LOGO_COLS, "");
			printf(LOGO_GAP);
		}
		if (i >= info_top && i < info_top + info_rows) {
			unsigned int k = i - info_top;

			if (k < lines->count)
				printf("%s", lines->text[k]);
			else if (k == lines->count + 1U)
				put_palette_row(40);
			else if (k == lines->count + 2U)
				put_palette_row(100);
		}
		printf("\n");
	}
	printf("\n");
}

/**
 * @brief Print the logo with the system information next to it.
 * @param[in] argc Number of command arguments.
 * @param[in] argv Argument vector; `--no-logo` or `-n` hides the logo.
 * @return Zero on success, one for an unknown option.
 */
int cmd_screenfetch(int argc, const char **argv)
{
	bool logo = true;
	int i;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--no-logo") == 0 || strcmp(argv[i], "-n") == 0) {
			logo = false;
		} else {
			printf("%s", cmd_screenfetch_usage);
			return 1;
		}
	}
	screenfetch(logo);
	return 0;
}
