#ifndef __SYS_UART_H__
#define __SYS_UART_H__

#include <byteorder.h>
#include <config.h>
#include <endian.h>
#include <io.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <types.h>

/*
 * Init System UART
 */
void sys_uart_init();

#endif // __SYS_UART_H__