#ifndef __FDT_WRAPPER_H__
#define __FDT_WRAPPER_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifndef __packed
#define __packed __attribute__((__packed__))
#endif

#ifndef __aligned
#define __aligned(x) __attribute__((__aligned__(x)))
#endif

#define MAX_LEVEL 32 /* how deeply nested we will go */
#define SCRATCHPAD 1024 /* bytes of scratchpad memory */
#define CMD_FDT_MAX_DUMP 64

/**
 * @brief Print the contents of the device tree at the specified path with a given property and depth.
 *
 * @param working_fdt The pointer to the device tree.
 * @param pathp The path of the node to start printing from.
 * @param prop The property name to print.
 * @param depth The maximum depth to traverse while printing.
 * @return The number of printed properties.
 */
int fdt_print(unsigned char *working_fdt, const char *pathp, const char *prop, int depth);

/**
 * @brief Parse the property values in the data buffer and return the new value pointers and lengths.
 *
 * @param newval The pointer to store the new value pointers.
 * @param count The number of properties in the data buffer.
 * @param data The buffer containing the property values.
 * @param len The pointer to store the length of each property value.
 * @return The number of parsed properties.
 */
int fdt_parse_prop(char const **newval, int count, char *data, int *len);

/**
 * @brief Increase the size of the flattened device tree by adding additional length.
 *
 * @param fdt The pointer to the flattened device tree.
 * @param add_len The additional length to be added to the device tree.
 * @return 0 success, other fail
 */
int fdt_increase_size(void *fdt, int add_len);

/**
 * @brief Find or add a subnode of a device-tree node.
 *
 * @param fdt Pointer to the device-tree blob.
 * @param parentoffset Structure-block offset of the parent node.
 * @param name Name of the subnode to locate.
 * @return The subnode offset on success, or a negative libfdt error code.
 */
int fdt_find_or_add_subnode(void *fdt, int parentoffset, const char *name);

/**
 * @brief Apply a device-tree overlay with verbose error reporting.
 *
 * @param fdt Pointer to the base device tree.
 * @param fdto Pointer to the device-tree overlay.
 * @return Zero on success, or a negative libfdt error code.
 */
int fdt_overlay_apply_verbose(void *fdt, void *fdto);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //__FDT_WRAPPER_H__
