/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file history.c
 * @brief Fixed-size circular history buffer for shell command lines.
 *
 * Entries are retained without dynamic allocation. Once the buffer wraps,
 * index zero refers to the newest retained line and older lines are evicted.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <types.h>

#include <cli/cli_history.h>

static char history[MSH_CMD_HISTORY_MAX][MSH_CMDLINE_CHAR_MAX];

static bool histfull;

static int histlast;

/**
 * @brief Get the number of command-history entries.
 *
 * @return The number of stored entries. The value is bounded by
 *         ::MSH_CMD_HISTORY_MAX and does not include an empty slot.
 */
int get_history_count()
{
	return histfull ? MSH_CMD_HISTORY_MAX : histlast;
}

/**
 * @brief Append a line to the command history.
 *
 * Empty lines and lines that exceed the history entry size are ignored.
 *
 * @param[in] line Null-terminated command line to append. It must remain
 *                 valid for the duration of the call.
 *
 * Lines of length zero or at least ::MSH_CMDLINE_CHAR_MAX are ignored, so the
 * stored copy always has room for its terminating NUL byte.
 */
void history_append(const char *line)
{
	int len = strlen(line); // Get the length of the input line.
	if (len >= MSH_CMDLINE_CHAR_MAX || len <= 0) { // If the line is too long or zero-length, ignore it.
		return;
	}

	strcpy(history[histlast], line); // Copy the input line to the history at the current index.

	if (histlast >= MSH_CMD_HISTORY_MAX - 1) { // If the history buffer is full.
		histfull = true; // Set the history buffer as full.
		histlast = 0; // Reset the index to the beginning of the history buffer.
	} else {
		histlast++; // Increment the index for the next history entry.
	}
}

/**
 * @brief Retrieve a command-history entry.
 *
 * @param[in] histnum Zero-based age of the entry, where zero selects the
 *                    newest retained line.
 * @return Pointer to the internal NUL-terminated entry, or `NULL` if the
 *         requested age is outside the retained history. The pointer remains
 *         valid until the next append that overwrites that slot.
 */
const char *history_get(int histnum)
{
	if (!histfull) { // If the history buffer is not full.
		if (histnum >= histlast) { // If the requested index is beyond the last entry.
			return NULL; // Return NULL as the entry doesn't exist.
		}
	} else if (histnum > MSH_CMD_HISTORY_MAX - 1 || histnum < 0) { // If the requested index is out of range.
		return NULL; // Return NULL as the entry doesn't exist.
	}

	if (histlast > histnum) { // If the requested index is within the currently stored entries.
		return (history[histlast - histnum - 1]); // Return the corresponding history entry.
	} else { // If the requested index refers to a wrapped-around entry.
		return history[MSH_CMD_HISTORY_MAX - (histnum - histlast) - 1]; // Return the wrapped-around history entry.
	}
}
