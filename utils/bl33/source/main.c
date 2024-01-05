#include <byteorder.h>
#include <endian.h>
#include <io.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

#include <rtc.h>

#define SUNXI_UART0_BASE 0x05000000

void sys_main(void) {
    sunxi_uart_init(SUNXI_UART0_BASE);
    
    sunxi_uart_putc('H');
    sunxi_uart_putc('e');
    sunxi_uart_putc('l');
    sunxi_uart_putc('l');
    sunxi_uart_putc('o');
    sunxi_uart_putc('W');
    sunxi_uart_putc('o');
    sunxi_uart_putc('r');
    sunxi_uart_putc('l');
    sunxi_uart_putc('d');
    sunxi_uart_putc('!');
    sunxi_uart_putc(' ');
    sunxi_uart_putc('f');
    sunxi_uart_putc('r');
    sunxi_uart_putc('o');
    sunxi_uart_putc('m');
    sunxi_uart_putc(' ');
    sunxi_uart_putc('a');
    sunxi_uart_putc('a');
    sunxi_uart_putc('r');
    sunxi_uart_putc('c');
    sunxi_uart_putc('h');
    sunxi_uart_putc('6');
    sunxi_uart_putc('4');
    sunxi_uart_putc('\n');
}
