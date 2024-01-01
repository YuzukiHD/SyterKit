/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __ELF_LOADER_H__
#define __ELF_LOADER_H__

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

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
#endif // __cplusplus

#endif// __ELF_LOADER_H__