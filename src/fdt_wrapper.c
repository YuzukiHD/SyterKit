/* SPDX-License-Identifier: GPL-2.0+ */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include "ctype.h"

#include "fdt_wrapper.h"
#include "libfdt.h"


/*
 * Parse the user's input, partially heuristic.  Valid formats:
 * <0x00112233 4 05>	- an array of cells.  Numbers follow standard
 *			C conventions.
 * [00 11 22 .. nn] - byte stream
 * "string"	- If the the value doesn't start with "<" or "[", it is
 *			treated as a string.  Note that the quotes are
 *			stripped by the parser before we get the string.
 * newval: An array of strings containing the new property as specified
 *	on the command line
 * count: The number of strings in the array
 * data: A bytestream to be placed in the property
 * len: The length of the resulting bytestream
 */
int fdt_parse_prop(char const **newval, int count, char *data, int *len) {
	char *cp;		   /* temporary char pointer */
	char *newp;		   /* temporary newval char pointer */
	unsigned long tmp; /* holds converted values */
	int stridx = 0;

	*len = 0;
	newp = newval[0];

	/* An array of cells */
	if (*newp == '<') {
		newp++;
		while ((*newp != '>') && (stridx < count)) {
			/*
			 * Keep searching until we find that last ">"
			 * That way users don't have to escape the spaces
			 */
			if (*newp == '\0') {
				newp = newval[++stridx];
				continue;
			}

			cp = newp;
			tmp = simple_strtoul(cp, &newp, 0);
			if (*cp != '?')
				*(fdt32_t *) data = cpu_to_fdt32(tmp);
			else
				newp++;

			data += 4;
			*len += 4;

			/* If the ptr didn't advance, something went wrong */
			if ((newp - cp) <= 0) {
				printk(LOG_LEVEL_MUTE, "Sorry, I could not convert \"%s\"\n", cp);
				return 1;
			}

			while (*newp == ' ') newp++;
		}

		if (*newp != '>') {
			printk(LOG_LEVEL_MUTE, "Unexpected character '%c'\n", *newp);
			return 1;
		}
	} else if (*newp == '[') {
		/*
		 * Byte stream.  Convert the values.
		 */
		newp++;
		while ((stridx < count) && (*newp != ']')) {
			while (*newp == ' ') newp++;
			if (*newp == '\0') {
				newp = newval[++stridx];
				continue;
			}
			if (!isxdigit(*newp))
				break;
			tmp = simple_strtoul(newp, &newp, 16);
			*data++ = tmp & 0xFF;
			*len = *len + 1;
		}
		if (*newp != ']') {
			printk(LOG_LEVEL_MUTE, "Unexpected character '%c'\n", *newp);
			return 1;
		}
	} else {
		/*
		 * Assume it is one or more strings.  Copy it into our
		 * data area for convenience (including the
		 * terminating '\0's).
		 */
		while (stridx < count) {
			size_t length = strlen(newp) + 1;
			strcpy(data, newp);
			data += length;
			*len += length;
			newp = newval[++stridx];
		}
	}
	return 0;
}

static int is_printable_string(const void *data, int len) {
	const char *s = data;

	/* zero length is not */
	if (len == 0)
		return 0;

	/* must terminate with zero or '\n' */
	if (s[len - 1] != '\0' && s[len - 1] != '\n')
		return 0;

	/* printable or a null byte (concatenated strings) */
	while (((*s == '\0') || isprint(*s) || isspace(*s)) && (len > 0)) {
		/*
		 * If we see a null, there are three possibilities:
		 * 1) If len == 1, it is the end of the string, printable
		 * 2) Next character also a null, not printable.
		 * 3) Next character not a null, continue to check.
		 */
		if (s[0] == '\0') {
			if (len == 1)
				return 1;
			if (s[1] == '\0')
				return 0;
		}
		s++;
		len--;
	}

	/* Not the null termination, or not done yet: not printable */
	if (*s != '\0' || (len != 0))
		return 0;

	return 1;
}

static void print_data(const void *data, int len) {
	int j;

	/* no data, don't print */
	if (len == 0)
		return;

	/*
	 * It is a string, but it may have multiple strings (embedded '\0's).
	 */
	if (is_printable_string(data, len)) {
		printk(LOG_LEVEL_MUTE, "\"");
		j = 0;
		while (j < len) {
			if (j > 0)
				printk(LOG_LEVEL_MUTE, "\", \"");
			printk(LOG_LEVEL_MUTE, data);
			j += strlen(data) + 1;
			data += strlen(data) + 1;
		}
		printk(LOG_LEVEL_MUTE, "\"");
		return;
	}

	if ((len % 4) == 0) {
		if (len > CMD_FDT_MAX_DUMP)
			printk(LOG_LEVEL_MUTE, "* 0x%p [0x%08x]", data, len);
		else {
			const uint32_t *p;

			printk(LOG_LEVEL_MUTE, "<");
			for (j = 0, p = data; j < len / 4; j++) printk(LOG_LEVEL_MUTE, "0x%08x%s", fdt32_to_cpu(p[j]), j < (len / 4 - 1) ? " " : "");
			printk(LOG_LEVEL_MUTE, ">");
		}
	} else { /* anything else... hexdump */
		if (len > CMD_FDT_MAX_DUMP)
			printk(LOG_LEVEL_MUTE, "* 0x%p [0x%08x]", data, len);
		else {
			const u8 *s;

			printk(LOG_LEVEL_MUTE, "[");
			for (j = 0, s = data; j < len; j++) printk(LOG_LEVEL_MUTE, "%02x%s", s[j], j < len - 1 ? " " : "");
			printk(LOG_LEVEL_MUTE, "]");
		}
	}
}

int fdt_print(unsigned char *working_fdt, const char *pathp, char *prop, int depth) {
	static char tabs[MAX_LEVEL + 1] = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
									  "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
	const void *nodep; /* property node pointer */
	int nodeoffset;	   /* node offset from libfdt */
	int nextoffset;	   /* next node offset from libfdt */
	uint32_t tag;	   /* tag */
	int len;		   /* length of the property */
	int level = 0;	   /* keep track of nesting level */
	const struct fdt_property *fdt_prop;

	nodeoffset = fdt_path_offset(working_fdt, pathp);
	if (nodeoffset < 0) {
		/*
		 * Not found or something else bad happened.
		 */
		printk(LOG_LEVEL_MUTE, "libfdt fdt_path_offset() returned %s\n", fdt_strerror(nodeoffset));
		return 1;
	}
	/*
	 * The user passed in a property as well as node path.
	 * Print only the given property and then return.
	 */
	if (prop) {
		nodep = fdt_getprop(working_fdt, nodeoffset, prop, &len);
		if (len == 0) {
			/* no property value */
			printk(LOG_LEVEL_MUTE, "%s %s\n", pathp, prop);
			return 0;
		} else if (nodep && len > 0) {
			printk(LOG_LEVEL_MUTE, "%s = ", prop);
			print_data(nodep, len);
			printk(LOG_LEVEL_MUTE, "\n");
			return 0;
		} else {
			printk(LOG_LEVEL_MUTE, "libfdt fdt_getprop(): %s\n", fdt_strerror(len));
			return 1;
		}
	}

	/*
	 * The user passed in a node path and no property,
	 * print the node and all subnodes.
	 */
	while (level >= 0) {
		tag = fdt_next_tag(working_fdt, nodeoffset, &nextoffset);
		switch (tag) {
			case FDT_BEGIN_NODE:
				pathp = fdt_get_name(working_fdt, nodeoffset, NULL);
				if (level <= depth) {
					if (pathp == NULL)
						pathp = "/* NULL pointer error */";
					if (*pathp == '\0')
						pathp = "/"; /* root is nameless */
					printk(LOG_LEVEL_MUTE, "%s%s {\n", &tabs[MAX_LEVEL - level], pathp);
				}
				level++;
				if (level >= MAX_LEVEL) {
					printk(LOG_LEVEL_MUTE, "Nested too deep, aborting.\n");
					return 1;
				}
				break;
			case FDT_END_NODE:
				level--;
				if (level <= depth)
					printk(LOG_LEVEL_MUTE, "%s};\n", &tabs[MAX_LEVEL - level]);
				if (level == 0) {
					level = -1; /* exit the loop */
				}
				break;
			case FDT_PROP:
				fdt_prop = fdt_offset_ptr(working_fdt, nodeoffset, sizeof(*fdt_prop));
				pathp = fdt_string(working_fdt, fdt32_to_cpu(fdt_prop->nameoff));
				len = fdt32_to_cpu(fdt_prop->len);
				nodep = fdt_prop->data;
				if (len < 0) {
					printk(LOG_LEVEL_MUTE, "libfdt fdt_getprop(): %s\n", fdt_strerror(len));
					return 1;
				} else if (len == 0) {
					/* the property has no value */
					if (level <= depth)
						printk(LOG_LEVEL_MUTE, "%s%s;\n", &tabs[MAX_LEVEL - level], pathp);
				} else {
					if (level <= depth) {
						printk(LOG_LEVEL_MUTE, "%s%s = ", &tabs[MAX_LEVEL - level], pathp);
						print_data(nodep, len);
						printk(LOG_LEVEL_MUTE, ";\n");
					}
				}
				break;
			case FDT_NOP:
				printk(LOG_LEVEL_MUTE, "%s/* NOP */\n", &tabs[MAX_LEVEL - level]);
				break;
			case FDT_END:
				return 1;
			default:
				if (level <= depth)
					printk(LOG_LEVEL_MUTE, "Unknown tag 0x%08X\n", tag);
				return 1;
		}
		nodeoffset = nextoffset;
	}
	return 0;
}

int fdt_increase_size(void *fdt, int add_len) {
	int newlen;
	newlen = fdt_totalsize(fdt) + add_len;
	/* Open in place with a new len */
	return fdt_open_into(fdt, fdt, newlen);
}

int fdt_find_or_add_subnode(void *fdt, int parent_offset, const char *name) {
	int offset;

	offset = fdt_subnode_offset(fdt, parent_offset, name);

	if (offset == -FDT_ERR_NOTFOUND)
		offset = fdt_add_subnode(fdt, parent_offset, name);

	if (offset < 0)
		printk_warning("FDT: find or add subnode %s: %s\n", name, fdt_strerror(offset));

	return offset;
}

int fdt_overlay_apply_verbose(void *fdt, void *fdto) {
	int err;
	bool has_symbols;

	err = fdt_path_offset(fdt, "/__symbols__");
	has_symbols = err >= 0;

	err = fdt_overlay_apply(fdt, fdto);
	if (err < 0) {
		printk_warning("failed on fdt_overlay_apply(): %s\n", fdt_strerror(err));
		if (!has_symbols) {
			printk_warning("base fdt does not have a /__symbols__ node\n");
			printk_warning("make sure you've compiled with -@\n");
		}
	}
	return err;
}