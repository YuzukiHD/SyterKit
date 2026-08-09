/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdint.h>

#include <backtrace.h>
#include <log.h>

void backtrace_print_begin(void) {
}

void backtrace_print_frame(uintptr_t address) {
#if __SIZEOF_POINTER__ == 8
	printk(LOG_LEVEL_BACKTRACE, "backtrace: 0x%016lx\n",
	       (unsigned long) address);
#else
	printk(LOG_LEVEL_BACKTRACE, "backtrace: 0x%08x\n",
	       (uint32_t) address);
#endif
}

void backtrace_print_end(void) {
}
