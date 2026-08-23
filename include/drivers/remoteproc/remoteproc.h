/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_REMOTEPROC_H__
#define __DRIVERS_REMOTEPROC_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <drivers/rtc/rtc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SUNXI_REMOTEPROC_MAX_FIRMWARES 4U
#define SUNXI_REMOTEPROC_MAX_ADDRESS_MAPS 4U
#define SUNXI_REMOTEPROC_MAX_REGISTERS 4U

typedef enum {
	SUNXI_REMOTEPROC_FIRMWARE_ELF32 = 0,
	SUNXI_REMOTEPROC_FIRMWARE_ELF64,
	SUNXI_REMOTEPROC_FIRMWARE_RAW,
} sunxi_remoteproc_firmware_format_t;

typedef struct {
	const char *name;
	uintptr_t load_address;
	size_t region_size;
} sunxi_remoteproc_firmware_t;

typedef struct {
	uintptr_t device_start;
	uintptr_t device_end;
	uintptr_t physical_start;
} sunxi_remoteproc_address_map_t;

typedef struct {
	uintptr_t base;
	size_t size;
} sunxi_remoteproc_register_t;

struct sunxi_remoteproc;

typedef struct {
	int (*reset)(struct sunxi_remoteproc *remoteproc);
	int (*prepare)(struct sunxi_remoteproc *remoteproc);
	int (*start)(struct sunxi_remoteproc *remoteproc);
	void (*dump)(const struct sunxi_remoteproc *remoteproc);
	int (*load_buffer)(struct sunxi_remoteproc *remoteproc,
			   const void *firmware, size_t size);
} sunxi_remoteproc_ops_t;

typedef struct sunxi_remoteproc {
	int dt_node;
	sunxi_remoteproc_firmware_format_t format;
	sunxi_remoteproc_firmware_t firmware[SUNXI_REMOTEPROC_MAX_FIRMWARES];
	size_t firmware_count;
	sunxi_remoteproc_address_map_t
		address_map[SUNXI_REMOTEPROC_MAX_ADDRESS_MAPS];
	size_t address_map_count;
	sunxi_remoteproc_register_t
		registers[SUNXI_REMOTEPROC_MAX_REGISTERS];
	size_t register_count;
	uintptr_t entry;
	bool entry_from_elf;
	sunxi_rtc_t *rtc;
	const sunxi_remoteproc_ops_t *ops;
} sunxi_remoteproc_t;

int sunxi_remoteproc_reset(sunxi_remoteproc_t *remoteproc);
int sunxi_remoteproc_load(sunxi_remoteproc_t *remoteproc);
int sunxi_remoteproc_load_buffer(sunxi_remoteproc_t *remoteproc,
				 const void *firmware, size_t size);
int sunxi_remoteproc_prepare(sunxi_remoteproc_t *remoteproc);
int sunxi_remoteproc_start(sunxi_remoteproc_t *remoteproc);
void sunxi_remoteproc_dump(const sunxi_remoteproc_t *remoteproc);

/* Implemented by the selected SoC remoteproc driver. */
extern const sunxi_remoteproc_ops_t sunxi_remoteproc_ops;

#ifdef __cplusplus
}
#endif

#endif /* __DRIVERS_REMOTEPROC_H__ */
