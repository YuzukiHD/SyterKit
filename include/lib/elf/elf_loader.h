/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __ELF_LOADER_H__
#define __ELF_LOADER_H__

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

/**
 * @brief The structure vaddr_range_t represents the mapping between a virtual address range and the starting physical address.
 */
typedef struct vaddr_range {
	uint64_t vstart; /**< The starting address of the virtual address range */
	uint64_t vend;	 /**< The ending address of the virtual address range */
	uint64_t pstart; /**< The starting position of the corresponding physical address */
} vaddr_range_t;

/**
 * @brief The structure vaddr_map_t represents the mapping between a virtual address range and the starting physical address and size of this table.
 */
typedef struct vaddr_map {
	vaddr_range_t *range;
	uint32_t range_size;
} vaddr_map_t;

/**
 * Extracts the entry address from an ELF32 image loaded at 'base'.
 *
 * @param base The base address of the ELF32 image.
 * @return The entry address of the ELF32 image.
 */
phys_addr_t elf32_get_entry_addr(phys_addr_t base);

/**
 * Loads an ELF32 image into memory starting at 'img_addr'.
 *
 * @param img_addr The starting address to load the ELF32 image.
 * @return 0 if successful, -1 otherwise.
 */
int load_elf32_image(phys_addr_t img_addr);

/**
 * Loads an ELF32 image into memory starting at 'img_addr', with remap va to pa
 *
 * @param img_addr The starting address to load the ELF32 image.
 * @param map The address mapping table.
 * @return 0 if successful, -1 otherwise.
 */
int load_elf32_image_remap(phys_addr_t img_addr, vaddr_map_t *map);

/**
 * Extracts the entry address from an ELF64 image loaded at 'base'.
 *
 * @param base The base address of the ELF64 image.
 * @return The entry address of the ELF64 image.
 */
phys_addr_t elf64_get_entry_addr(phys_addr_t base);

/**
 * Loads an ELF64 image into memory starting at 'img_addr'.
 *
 * @param img_addr The starting address to load the ELF64 image.
 * @return 0 if successful, -1 otherwise.
 */
int load_elf64_image(phys_addr_t img_addr);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __ELF_LOADER_H__