#ifndef __FDT_WRAPPER_H__
#define __FDT_WRAPPER_H__

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

#ifndef __packed
#define __packed __attribute__((__packed__))
#endif

#ifndef __aligned
#define __aligned(x) __attribute__((__aligned__(x)))
#endif

#define MAX_LEVEL 32	/* how deeply nested we will go */
#define SCRATCHPAD 1024 /* bytes of scratchpad memory */
#define CMD_FDT_MAX_DUMP 64

/**
 * Print the contents of the device tree at the specified path with a given property and depth.
 *
 * @param working_fdt The pointer to the device tree.
 * @param pathp The path of the node to start printing from.
 * @param prop The property name to print.
 * @param depth The maximum depth to traverse while printing.
 * @return The number of printed properties.
 */
int fdt_print(unsigned char *working_fdt, const char *pathp, char *prop, int depth);

/**
 * Parse the property values in the data buffer and return the new value pointers and lengths.
 *
 * @param newval The pointer to store the new value pointers.
 * @param count The number of properties in the data buffer.
 * @param data The buffer containing the property values.
 * @param len The pointer to store the length of each property value.
 * @return The number of parsed properties.
 */
int fdt_parse_prop(char const **newval, int count, char *data, int *len);

/**
 * Increase the size of the flattened device tree by adding additional length.
 *
 * @param fdt The pointer to the flattened device tree.
 * @param add_len The additional length to be added to the device tree.
 * @return 0 success, other fail
 */
int fdt_increase_size(void *fdt, int add_len);

/**
 * fdt_find_or_add_subnode() - find or possibly add a subnode of a given node
 *
 * @fdt: pointer to the device tree blob
 * @parentoffset: structure block offset of a node
 * @name: name of the subnode to locate
 *
 * fdt_subnode_offset() finds a subnode of the node with a given name.
 * If the subnode does not exist, it will be created.
 */
int fdt_find_or_add_subnode(void *fdt, int parentoffset, const char *name);

/**
 * fdt_overlay_apply_verbose - Apply an overlay with verbose error reporting
 *
 * @fdt: ptr to device tree
 * @fdto: ptr to device tree overlay
 *
 * Convenience function to apply an overlay and display helpful messages
 * in the case of an error
 */
int fdt_overlay_apply_verbose(void *fdt, void *fdto);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif//__FDT_WRAPPER_H__