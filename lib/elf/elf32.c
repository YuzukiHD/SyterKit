/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <types.h>

#include <elf.h>
#include <elf_helpers.h>
#include <elf_loader.h>

#include <log.h>

/* Default not mapping, empty mapping table */
static vaddr_map_t default_addr_mapping = {
		.range = (vaddr_range_t *) NULL,
		.range_size = 0,
};

void print_elf32_ehdr(Elf32_Ehdr *header) {
	printk_info("e_ident: ");
	for (int i = 0; i < EI_NIDENT; i++) { printk(LOG_LEVEL_MUTE, "%02x ", header->e_ident[i]); }
	printk(LOG_LEVEL_MUTE, "\n");
	printk_debug("e_type: 0x%08x\n", header->e_type);
	printk_debug("e_machine: 0x%08x\n", header->e_machine);
	printk_debug("e_version: 0x%08x\n", header->e_version);
	printk_debug("e_entry: 0x%08x\n", header->e_entry);
	printk_debug("e_phoff: 0x%08x\n", header->e_phoff);
	printk_debug("e_shoff: 0x%08x\n", header->e_shoff);
	printk_debug("e_flags: 0x%08x\n", header->e_flags);
	printk_debug("e_ehsize: 0x%08x\n", header->e_ehsize);
	printk_debug("e_phentsize: 0x%08x\n", header->e_phentsize);
	printk_debug("e_phnum: 0x%08x\n", header->e_phnum);
	printk_debug("e_shentsize: 0x%08x\n", header->e_shentsize);
	printk_debug("e_shnum: 0x%08x\n", header->e_shnum);
	printk_debug("e_shstrndx: 0x%08x\n", header->e_shstrndx);
}

/**
 * @brief Get the entry address from the ELF32 file header
 * 
 * @param base Base address of the ELF32 file
 * @return Entry address
 */
phys_addr_t elf32_get_entry_addr(phys_addr_t base) {
	Elf32_Ehdr *ehdr;
	ehdr = (Elf32_Ehdr *) base;
	return ehdr->e_entry;
}

/**
 * @brief Map virtual address to physical address
 * 
 * @param vaddr Virtual address to be mapped
 * @param map Mapping table containing virtual address range and corresponding physical address range
 * @param size Size of the mapping table
 * @return Mapped physical address
 */
static phys_addr_t set_img_va_to_pa(phys_addr_t vaddr, vaddr_range_t *map, uint32_t size) {
	phys_addr_t paddr = vaddr;
	for (uint32_t i = 0; i < size; i++) {
		if (vaddr >= map[i].vstart && vaddr <= map[i].vend) {
			paddr = paddr - map[i].vstart + map[i].pstart;
			break;
		}
	}
	return paddr;
}

int load_elf32_image(phys_addr_t img_addr) {
	return load_elf32_image_remap(img_addr, &default_addr_mapping);
}

int load_elf32_image_remap(phys_addr_t img_addr, vaddr_map_t *map) {
	Elf32_Ehdr *ehdr = NULL;
	Elf32_Phdr *phdr = NULL;
	void *dst = NULL;
	void *src = NULL;

	ehdr = (Elf32_Ehdr *) img_addr;

	print_elf32_ehdr(ehdr);

	phdr = (Elf32_Phdr *) (img_addr + ehdr->e_phoff);

	/* load elf program segment */
	for (int i = 0; i < ehdr->e_phnum; ++i, ++phdr) {
		/* remap addresses */
		dst = (void *) (phys_addr_t) set_img_va_to_pa((phys_addr_t) phdr->p_paddr, map->range, map->range_size);
		src = (void *) (img_addr + phdr->p_offset);

		printk_debug("ELF: Loading phdr %i from 0x%x to 0x%x (%i bytes)\n", i, phdr->p_paddr, dst, phdr->p_filesz);

		if (phdr->p_type != PT_LOAD)
			continue;

		if ((phdr->p_memsz == 0) || (phdr->p_filesz == 0))
			continue;

		if (phdr->p_filesz)
			memcpy(dst, src, phdr->p_filesz);

		if (phdr->p_filesz != phdr->p_memsz)
			memset((u8 *) dst + phdr->p_filesz, 0x00, phdr->p_memsz - phdr->p_filesz);
	}

	return 0;
}

static Elf32_Shdr *elf32_find_segment(phys_addr_t elf_addr, const char *seg_name) {
	int i = 0;
	Elf32_Shdr *shdr;
	Elf32_Ehdr *ehdr;
	const char *name_table;
	const uint8_t *elf_data = (void *) elf_addr;

	ehdr = (Elf32_Ehdr *) elf_data;
	shdr = (Elf32_Shdr *) (elf_data + ehdr->e_shoff);
	name_table = (const char *) (elf_data + shdr[ehdr->e_shstrndx].sh_offset);

	for (i = 0; i < ehdr->e_shnum; i++, shdr++) {
		if (strcmp(name_table + shdr->sh_name, seg_name))
			continue;

		return shdr;
	}

	return NULL;
}

void *elf32_find_segment_offset(phys_addr_t elf_addr, const char *seg_name) {
	Elf32_Shdr *shdr;

	shdr = elf32_find_segment(elf_addr, seg_name);
	if (!shdr)
		return NULL;

	return (void *) elf_addr + shdr->sh_offset;
}

void *elf32_find_segment_addr(phys_addr_t elf_addr, const char *seg_name) {
	Elf32_Shdr *shdr;

	shdr = elf32_find_segment(elf_addr, seg_name);
	if (!shdr)
		return NULL;

	return (void *) shdr->sh_addr;
}
