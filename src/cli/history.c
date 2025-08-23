/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <types.h>

#include "cli_history.h"

static char history[MSH_CMD_HISTORY_MAX][MSH_CMDLINE_CHAR_MAX];

static bool histfull;

static int histlast;

/* 
    Get the count of history entries.
    Returns: The count of history entries.
*/
int get_history_count() {
	return histlast;// Return the index of the last history entry.
}

/*
    Append a line to the command history.
    Parameters:
    - line: The line to be appended to the history.
    Notes:
    If a given string is too long or zero-length, it will be ignored.
*/
void history_append(const char *line) {
	int len = strlen(line);						  // Get the length of the input line.
	if (len >= MSH_CMDLINE_CHAR_MAX || len <= 0) {// If the line is too long or zero-length, ignore it.
		return;
	}

	strcpy(history[histlast], line);// Copy the input line to the history at the current index.

	if (histlast >= MSH_CMD_HISTORY_MAX - 1) {// If the history buffer is full.
		histfull = true;					  // Set the history buffer as full.
		histlast = 0;						  // Reset the index to the beginning of the history buffer.
	} else {
		histlast++;// Increment the index for the next history entry.
	}
}

/*
 *  Retrieve a specific history entry.
 *  Parameters:
 *  - histnum: The index of the history entry to retrieve.
 * Returns: The history entry at the specified index.
 */
const char *history_get(int histnum) {
	if (!histfull) {			  // If the history buffer is not full.
		if (histnum >= histlast) {// If the requested index is beyond the last entry.
			return NULL;		  // Return NULL as the entry doesn't exist.
		}
	} else if (histnum > MSH_CMD_HISTORY_MAX - 1 || histnum < 0) {// If the requested index is out of range.
		return NULL;											  // Return NULL as the entry doesn't exist.
	}

	if (histlast > histnum) {										   // If the requested index is within the currently stored entries.
		return (history[histlast - histnum - 1]);					   // Return the corresponding history entry.
	} else {														   // If the requested index refers to a wrapped-around entry.
		return history[MSH_CMD_HISTORY_MAX - (histnum - histlast) - 1];// Return the wrapped-around history entry.
	}
}
