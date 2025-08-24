#include <byteorder.h>
#include <config.h>
#include <endian.h>
#include <io.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <types.h>


void sys_uart_putc(char c) {
	virtual_addr_t addr = UART0_BASE_ADDR;

	while ((read32(addr + 0x7c) & (0x1 << 1)) == 0)
		;
	write32(addr + 0x00, c);
}

static int v_printf_str_to_num(const char *fmt, int *num) {
	const char *p;
	int res, d, isd;

	res = 0;
	for (p = fmt; *fmt != '\0'; p++) {
		isd = (*p >= '0' && *p <= '9');
		if (!isd)
			break;
		d = *p - '0';
		res *= 10;
		res += d;
	}
	*num = res;
	return ((int) (p - fmt));
}

static void v_printf_num_to_str(uint32_t a, int ish, int pl, int pc) {
	char buf[32];
	uint32_t base;
	int idx, i, t;

	for (i = 0; i < sizeof(buf); i++) buf[i] = pc;
	base = 10;
	if (ish)
		base = 16;

	idx = 0;
	do {
		t = a % base;
		if (t >= 10)
			buf[idx] = t - 10 + 'a';
		else
			buf[idx] = t + '0';
		a /= base;
		idx++;
	} while (a > 0);

	if (pl > 0) {
		if (pl >= sizeof(buf))
			pl = sizeof(buf) - 1;
		if (idx < pl)
			idx = pl;
	}
	buf[idx] = '\0';

	for (i = idx - 1; i >= 0; i--) sys_uart_putc(buf[i]);
}

static int v_printf(const char *fmt, va_list va) {
	const char *p, *q;
	int f, c, vai, pl, pc, i;
	unsigned char t;

	pc = ' ';
	for (p = fmt; *p != '\0'; p++) {
		f = 0;
		pl = 0;
		c = *p;
		q = p;
		if (*p == '%') {
			q = p;
			p++;
			if (*p >= '0' && *p <= '9')
				p += v_printf_str_to_num(p, &pl);
			f = *p;
		}
		if ((f == 'd') || (f == 'x')) {
			vai = va_arg(va, int);
			v_printf_num_to_str(vai, f == 'x', pl, pc);
		} else {
			for (i = 0; i < (p - q); i++) sys_uart_putc(q[i]);
			t = (unsigned char) (f != 0 ? f : c);
			sys_uart_putc(t);
		}
	}
	return 0;
}

int sys_uart_printf(const char *fmt, ...) {
	va_list va;

	va_start(va, fmt);
	v_printf(fmt, va);
	va_end(va);
	return 0;
}
