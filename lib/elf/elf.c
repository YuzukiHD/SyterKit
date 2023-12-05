#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <types.h>

#include <elf.h>
#include <elf_helpers.h>
#include <elf_loader.h>

#include <log.h>

void print_elf32_ehdr(Elf32_Ehdr *header)
{
	printk(LOG_LEVEL_DEBUG, "e_ident: ");
	for (int i = 0; i < EI_NIDENT; i++) {
		printk(LOG_LEVEL_MUTE, "%02x ", header->e_ident[i]);
	}
	printk(LOG_LEVEL_MUTE, "\r\n");
	printk(LOG_LEVEL_DEBUG, "e_type: 0x%08x\r\n", header->e_type);
	printk(LOG_LEVEL_DEBUG, "e_machine: 0x%08x\r\n", header->e_machine);
	printk(LOG_LEVEL_DEBUG, "e_version: 0x%08x\r\n", header->e_version);
	printk(LOG_LEVEL_DEBUG, "e_entry: 0x%08x\r\n", header->e_entry);
	printk(LOG_LEVEL_DEBUG, "e_phoff: 0x%08x\r\n", header->e_phoff);
	printk(LOG_LEVEL_DEBUG, "e_shoff: 0x%08x\r\n", header->e_shoff);
	printk(LOG_LEVEL_DEBUG, "e_flags: 0x%08x\r\n", header->e_flags);
	printk(LOG_LEVEL_DEBUG, "e_ehsize: 0x%08x\r\n", header->e_ehsize);
	printk(LOG_LEVEL_DEBUG, "e_phentsize: 0x%08x\r\n", header->e_phentsize);
	printk(LOG_LEVEL_DEBUG, "e_phnum: 0x%08x\r\n", header->e_phnum);
	printk(LOG_LEVEL_DEBUG, "e_shentsize: 0x%08x\r\n", header->e_shentsize);
	printk(LOG_LEVEL_DEBUG, "e_shnum: 0x%08x\r\n", header->e_shnum);
	printk(LOG_LEVEL_DEBUG, "e_shstrndx: 0x%08x\r\n", header->e_shstrndx);
}

phys_addr_t elf_get_entry_addr(phys_addr_t base)
{
	Elf32_Ehdr *ehdr;

	ehdr = (Elf32_Ehdr *)base;

	return ehdr->e_entry;
}

int load_elf_image(phys_addr_t img_addr)
{
	int i;
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;

	void *dst = NULL;
	void *src = NULL;

	ehdr = (Elf32_Ehdr *)img_addr;

	print_elf32_ehdr(ehdr);

	phdr = (Elf32_Phdr *)(img_addr + ehdr->e_phoff);

	/* load elf program segment */
	for (i = 0; i < ehdr->e_phnum; ++i, ++phdr) {
		dst = (void *)((phys_addr_t)phdr->p_paddr);
		src = (void *)(img_addr + phdr->p_offset);

		if (phdr->p_type != PT_LOAD)
			continue;

		if ((phdr->p_memsz == 0) || (phdr->p_filesz == 0))
			continue;

		if (phdr->p_filesz)
			memcpy(dst, src, phdr->p_filesz);

		if (phdr->p_filesz != phdr->p_memsz)
			memset((u8 *)dst + phdr->p_filesz, 0x00,
			       phdr->p_memsz - phdr->p_filesz);
	}

	return 0;
}

static Elf32_Shdr *elf_find_segment(phys_addr_t elf_addr, const char *seg_name)
{
	int i = 0;
	Elf32_Shdr *shdr;
	Elf32_Ehdr *ehdr;
	const char *name_table;
	const uint8_t *elf_data = (void *)elf_addr;

	ehdr = (Elf32_Ehdr *)elf_data;
	shdr = (Elf32_Shdr *)(elf_data + ehdr->e_shoff);
	name_table =
		(const char *)(elf_data + shdr[ehdr->e_shstrndx].sh_offset);

	for (i = 0; i < ehdr->e_shnum; i++, shdr++) {
		if (strcmp(name_table + shdr->sh_name, seg_name))
			continue;

		return shdr;
	}

	return NULL;
}

void *elf_find_segment_offset(phys_addr_t elf_addr, const char *seg_name)
{
	Elf32_Shdr *shdr;

	shdr = elf_find_segment(elf_addr, seg_name);
	if (!shdr)
		return NULL;

	return (void *)elf_addr + shdr->sh_offset;
}

void *elf_find_segment_addr(phys_addr_t elf_addr, const char *seg_name)
{
	Elf32_Shdr *shdr;

	shdr = elf_find_segment(elf_addr, seg_name);
	if (!shdr)
		return NULL;

	return (void *)shdr->sh_addr;
}
