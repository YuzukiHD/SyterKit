#ifndef __FDT_WRAPPER_H__
#define __FDT_WRAPPER_H__


#define MAX_LEVEL 32    /* how deeply nested we will go */
#define SCRATCHPAD 1024 /* bytes of scratchpad memory */
#define CMD_FDT_MAX_DUMP 64

int fdt_print(unsigned char *working_fdt, const char *pathp, char *prop, int depth);

int fdt_parse_prop(char const **newval, int count, char *data, int *len);

#endif //__FDT_WRAPPER_H__