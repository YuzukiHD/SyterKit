#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))
#define ALIGN(x, a) __ALIGN_MASK((x), (typeof(x)) (a) -1)

struct boot_head_t {
	uint32_t instruction;
	uint8_t magic[8];
	uint32_t checksum;
	uint32_t length;
	uint8_t spl_signature[4];
	uint32_t fel_script_address;
	uint32_t fel_uenv_length;
	uint32_t dt_name_offset;
	uint32_t reserved1;
	uint32_t boot_media;
	uint32_t string_pool[13];
};

int main(int argc, char *argv[]) {
	struct boot_head_t *h;
	FILE *fp;
	char *buffer;
	int buflen, filelen;
	uint32_t *p;
	uint32_t sum = 0;
	int i = 0, l = 0, loop = 0, padding = 0;

	if (argc != 3) {
		printf("Usage: mksunxi <bootloader> <padding>\n");
		return -1;
	}

	padding = atoi(argv[2]);
	printf("padding: %d\n", padding);

	if (padding != 512 && padding != 8192) {
		printf("padding must be 512 (block devices) or 8192 (flash)\n");
		exit(1);
	}

	fp = fopen(argv[1], "r+b");
	if (fp == NULL) {
		printf("Open bootloader error\n");
		return -1;
	}
	fseek(fp, 0L, SEEK_END);
	filelen = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	if (filelen <= sizeof(struct boot_head_t)) {
		fclose(fp);
		printf("The size of bootloader too small\n");
		return -1;
	}

	buflen = ALIGN(filelen, padding);
	buffer = malloc(buflen);
	memset(buffer, 0, buflen);
	if (fread(buffer, 1, filelen, fp) != filelen) {
		printf("Can't read bootloader\n");
		free(buffer);
		fclose(fp);
		return -1;
	}

	h = (struct boot_head_t *) buffer;
	p = (uint32_t *) h;
	l = (h->length);
	printf("len: %u\n", l);
	l = ALIGN(l, padding);
	h->length = (l);
	h->checksum = (0x5F0A6C39);
	loop = l >> 2;
	for (i = 0, sum = 0; i < loop; i++) sum += (p[i]);
	h->checksum = (sum);

	fseek(fp, 0L, SEEK_SET);
	if (fwrite(buffer, 1, buflen, fp) != buflen) {
		printf("Write bootloader error\n");
		free(buffer);
		fclose(fp);
		return -1;
	}

	fclose(fp);
	printf("The bootloader head has been fixed, spl size is %d bytes.\n", l);
	return 0;
}
