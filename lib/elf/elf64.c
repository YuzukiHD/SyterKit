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

void print_elf64_ehdr(Elf64_Ehdr *header) {
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

phys_addr_t elf64_get_entry_addr(phys_addr_t base) {
	Elf64_Ehdr *ehdr;

	ehdr = (Elf64_Ehdr *) base;

	return ehdr->e_entry;
}

int load_elf64_image(phys_addr_t img_addr) {
	int i;
	Elf64_Ehdr *ehdr;
	Elf64_Phdr *phdr;

	void *dst = NULL;
	void *src = NULL;

	ehdr = (Elf64_Ehdr *) img_addr;

	print_elf64_ehdr(ehdr);

	phdr = (Elf64_Phdr *) (img_addr + ehdr->e_phoff);

	/* load elf program segment */
	for (i = 0; i < ehdr->e_phnum; ++i, ++phdr) {
		dst = (void *) ((phys_addr_t) phdr->p_paddr);
		src = (void *) (img_addr + phdr->p_offset);

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

static Elf64_Shdr *elf64_find_segment(phys_addr_t elf_addr, const char *seg_name) {
	int i = 0;
	Elf64_Shdr *shdr;
	Elf64_Ehdr *ehdr;
	const char *name_table;
	const uint8_t *elf_data = (void *) elf_addr;

	ehdr = (Elf64_Ehdr *) elf_data;
	shdr = (Elf64_Shdr *) (elf_data + ehdr->e_shoff);
	name_table = (const char *) (elf_data + shdr[ehdr->e_shstrndx].sh_offset);

	for (i = 0; i < ehdr->e_shnum; i++, shdr++) {
		if (strcmp(name_table + shdr->sh_name, seg_name))
			continue;

		return shdr;
	}

	return NULL;
}

void *elf64_find_segment_offset(phys_addr_t elf_addr, const char *seg_name) {
	Elf64_Shdr *shdr;

	shdr = elf64_find_segment(elf_addr, seg_name);
	if (!shdr)
		return NULL;

	return (void *) elf_addr + shdr->sh_offset;
}

void *elf64_find_segment_addr(phys_addr_t elf_addr, const char *seg_name) {
	Elf64_Shdr *shdr;

	shdr = elf64_find_segment(elf_addr, seg_name);
	if (!shdr)
		return NULL;

	return (void *) shdr->sh_addr;
}
