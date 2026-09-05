/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file remoteproc.h
 * @brief Remote processor (remoteproc) framework for Sunxi SoCs.
 *
 * Defines the firmware, address map and register descriptions together with
 * the operations used to reset, load, prepare and start a remote processor.
 */

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

/**
 * @enum sunxi_remoteproc_firmware_format_t
 * @brief Remote processor firmware image format.
 */
typedef enum {
	SUNXI_REMOTEPROC_FIRMWARE_ELF32 = 0, /**< 32-bit ELF firmware image. */
	SUNXI_REMOTEPROC_FIRMWARE_ELF64, /**< 64-bit ELF firmware image. */
	SUNXI_REMOTEPROC_FIRMWARE_RAW, /**< Raw binary firmware image. */
} sunxi_remoteproc_firmware_format_t;

/**
 * @struct sunxi_remoteproc_firmware_t
 * @brief Description of one remote processor firmware region.
 */
typedef struct {
	const char *name; /**< Firmware name, used for identification. */
	uintptr_t load_address; /**< Address where the firmware is loaded. */
	size_t region_size; /**< Size of the firmware region, in bytes. */
} sunxi_remoteproc_firmware_t;

/**
 * @struct sunxi_remoteproc_address_map_t
 * @brief Device-to-physical address mapping for a remote processor.
 */
typedef struct {
	uintptr_t device_start; /**< Start address on the remote processor device bus. */
	uintptr_t device_end; /**< End address on the remote processor device bus. */
	uintptr_t physical_start; /**< Corresponding host physical start address. */
} sunxi_remoteproc_address_map_t;

/**
 * @struct sunxi_remoteproc_register_t
 * @brief Register window description for a remote processor.
 */
typedef struct {
	uintptr_t base; /**< Base address of the register window. */
	size_t size; /**< Size of the register window, in bytes. */
} sunxi_remoteproc_register_t;

struct sunxi_remoteproc;

/**
 * @struct sunxi_remoteproc_ops_t
 * @brief Operations implemented by the SoC-specific remoteproc driver.
 */
typedef struct {
	int (*reset)(struct sunxi_remoteproc *remoteproc); /**< Reset the remote processor. */
	int (*prepare)(struct sunxi_remoteproc *remoteproc); /**< Prepare the remote processor for start. */
	int (*start)(struct sunxi_remoteproc *remoteproc); /**< Start the remote processor. */
	void (*dump)(const struct sunxi_remoteproc *remoteproc); /**< Dump remote processor state. */
	int (*load_buffer)(struct sunxi_remoteproc *remoteproc, const void *firmware, size_t size); /**< Load a firmware buffer. */
} sunxi_remoteproc_ops_t;

/**
 * @struct sunxi_remoteproc_t
 * @brief Remote processor instance description.
 */
typedef struct sunxi_remoteproc {
	int dt_node; /**< Device-tree node describing the remote processor. */
	sunxi_remoteproc_firmware_format_t format; /**< Firmware image format. */
	sunxi_remoteproc_firmware_t firmware[SUNXI_REMOTEPROC_MAX_FIRMWARES]; /**< Firmware region descriptions. */
	size_t firmware_count; /**< Number of valid firmware regions. */
	sunxi_remoteproc_address_map_t address_map[SUNXI_REMOTEPROC_MAX_ADDRESS_MAPS]; /**< Device-to-physical address maps. */
	size_t address_map_count; /**< Number of valid address maps. */
	sunxi_remoteproc_register_t registers[SUNXI_REMOTEPROC_MAX_REGISTERS]; /**< Register windows. */
	size_t register_count; /**< Number of valid register windows. */
	uintptr_t entry; /**< Entry point address for the remote processor. */
	bool entry_from_elf; /**< Whether the entry point was taken from the ELF header. */
	sunxi_rtc_t *rtc; /**< RTC used for reset and start sequencing. */
	const sunxi_remoteproc_ops_t *ops; /**< SoC-specific remoteproc operations. */
} sunxi_remoteproc_t;

/**
 * @brief Reset the remote processor.
 */
int sunxi_remoteproc_reset(sunxi_remoteproc_t *remoteproc);

/**
 * @brief Load all configured firmware regions into the remote processor.
 */
int sunxi_remoteproc_load(sunxi_remoteproc_t *remoteproc);

/**
 * @brief Load a single firmware buffer into the remote processor.
 */
int sunxi_remoteproc_load_buffer(sunxi_remoteproc_t *remoteproc, const void *firmware, size_t size);

/**
 * @brief Prepare the remote processor for starting.
 */
int sunxi_remoteproc_prepare(sunxi_remoteproc_t *remoteproc);

/**
 * @brief Start the remote processor.
 */
int sunxi_remoteproc_start(sunxi_remoteproc_t *remoteproc);

/**
 * @brief Dump the remote processor state.
 */
void sunxi_remoteproc_dump(const sunxi_remoteproc_t *remoteproc);

/* Implemented by the selected SoC remoteproc driver. */
extern const sunxi_remoteproc_ops_t sunxi_remoteproc_ops;

#ifdef __cplusplus
}
#endif

#endif /* __DRIVERS_REMOTEPROC_H__ */
