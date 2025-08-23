#include <byteorder.h>
#include <config.h>
#include <endian.h>
#include <io.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys-clock.h>
#include <sys-uart.h>
#include <types.h>
#include <uart.h>

#define OPENSBI_FW_TEXT_START 0x41fc0000

void jmp_opensbi(uint32_t opensbi_base) {
	asm volatile("jr a0");
__LOOP:
	asm volatile("WFI");
	goto __LOOP;
}

int main() {
	int i = 0;

	sys_uart_init();// init UART0

	sys_uart_printf(" _____     _           _____ _ _      _____ ___ ___ ___ \r\n");
	sys_uart_printf("|   __|_ _| |_ ___ ___|  |  |_| |_   |     | . |   |  _|\r\n");
	sys_uart_printf("|__   | | |  _| -_|  _|    -| |  _|  |   --|_  | | | . |\r\n");
	sys_uart_printf("|_____|_  |_| |___|_| |__|__|_|_|    |_____|___|___|___|\r\n");
	sys_uart_printf("      |___| \r\n\r\n");

	sys_uart_printf("This Message is from C906 RISC-V Core\r\n");

	sys_uart_printf("Countting to 9\r\nCount: ");

	for (i = 0; i < 9; i++) {
		sys_uart_printf("%d ", i);
		sdelay(100000);
	}

	sys_uart_printf("Jump to OpenSBI...\r\n");

	jmp_opensbi(OPENSBI_FW_TEXT_START);
}
