#ifndef __STDLIB_H__
#define __STDLIB_H__

#ifdef __cplusplus
extern "C" {
#endif

void uart_printf(const char *fmt, ...);

void printf(const char *fmt, ...);

uint32_t time_ms(void);

#ifdef __cplusplus
}
#endif

#endif /* __STDLIB_H__ */
