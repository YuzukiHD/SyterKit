#include <byteorder.h>
#include <endian.h>
#include <io.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys-clock.h>
#include <types.h>

static inline uint64_t counter() {
	uint64_t cnt;
	__asm__ __volatile__("csrr %0, time\n"
						 : "=r"(cnt)::"memory");
	return cnt;
}

void sdelay(unsigned long us) {
	uint64_t t1 = counter();
	uint64_t t2 = t1 + us * 24;
	do { t1 = counter(); } while (t2 >= t1);
}
