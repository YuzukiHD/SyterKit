/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __ELF_LOADER_H__
#define __ELF_LOADER_H__

phys_addr_t elf_get_entry_addr(phys_addr_t base);

int load_elf_image(phys_addr_t img_addr);

#endif // __ELF_LOADER_H__