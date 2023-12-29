#ifndef __FDT_WRAPPER_H__
#define __FDT_WRAPPER_H__

#ifdef __cplusplus
extern "C" { 
#endif // __cplusplus

#ifndef __packed
#define __packed	__attribute__((__packed__))
#endif

#ifndef __aligned
#define __aligned(x)	__attribute__((__aligned__(x)))
#endif

#define MAX_LEVEL 32    /* how deeply nested we will go */
#define SCRATCHPAD 1024 /* bytes of scratchpad memory */
#define CMD_FDT_MAX_DUMP 64

int fdt_print(unsigned char *working_fdt, const char *pathp, char *prop, int depth);

int fdt_parse_prop(char const **newval, int count, char *data, int *len);

int fdt_increase_size(void *fdt, int add_len);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //__FDT_WRAPPER_H__