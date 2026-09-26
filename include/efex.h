/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SYTERKIT_EFEX_H__
#define __SYTERKIT_EFEX_H__

/**
 * @file efex.h
 * @brief eFEX runtime parameter area (ABI v1).
 *
 * Every eFEX image links one 4 KiB parameter area.  The eGON head points to
 * it twice: the descriptor at image + 0x30 ("SKEP") and the Boot ROM-visible
 * ret_addr.  A host may patch the area in the image file or write it through
 * FEL after download and before exec.  The application applies the whole
 * table once with efex_param_load() and writes its results back into the same
 * area before returning.
 *
 * The area is a 32-byte header followed by {key, value} pairs:
 *
 *   key[31]    EFEX_PARAM_APPLIED  set by the target when it applied the entry
 *   key[30]    EFEX_PARAM_OUTPUT   set by the target on entries it wrote
 *   key[29:24] group
 *   key[23:8]  id (field, or PMU number for rails)
 *   key[7:0]   index (array word, or rail number)
 *
 * Application-defined keys use key[23:0] as one number, see EFEX_PARAM_APP().
 *
 * All fields are little-endian.  docs/efex-param.md describes the host flow.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <drivers/dram/dram.h>
#include <drivers/i2c/i2c.h>
#include <drivers/psram/psram.h>
#include <drivers/serial/serial.h>

#define EFEX_PARAM_DESC_MAGIC "SKEP"
#define EFEX_PARAM_MAGIC      0x41504b53U /* "SKPA" */
#define EFEX_PARAM_VERSION    1U
#define EFEX_PARAM_AREA_SIZE  4096U

/* Target status written into efex_param_hdr.status. */
#define EFEX_PARAM_STATUS_IDLE 0U
#define EFEX_PARAM_STATUS_DONE 0x454e4f44U /* "DONE" */
#define EFEX_PARAM_STATUS_FAIL 0x4c494146U /* "FAIL" */
#define EFEX_PARAM_STATUS_BAD  0x50444142U /* "BADP": host table rejected */

/* Descriptor stored in the eGON head at image + 0x30. */
struct efex_param_desc {
	uint8_t magic[4];
	uint32_t addr;
	uint32_t size;
	uint32_t version;
};

struct efex_param_hdr {
	uint32_t magic;
	uint16_t version;
	uint16_t hdr_size;
	uint16_t count;
	uint16_t capacity;
	uint32_t checksum; /* 32-bit sum of all entry words, 0 = unchecked */
	uint32_t status;
	int32_t ret; /* main() return value */
	uint32_t reserved[2];
};

struct efex_param_ent {
	uint32_t key;
	uint32_t value;
};

#define EFEX_PARAM_CAPACITY ((EFEX_PARAM_AREA_SIZE - sizeof(struct efex_param_hdr)) / sizeof(struct efex_param_ent))

struct efex_param_area {
	struct efex_param_hdr hdr;
	struct efex_param_ent ent[EFEX_PARAM_CAPACITY];
};

_Static_assert(sizeof(struct efex_param_desc) == 16, "eFEX parameter descriptor must be 16 bytes");
_Static_assert(sizeof(struct efex_param_hdr) == 32, "eFEX parameter header must be 32 bytes");
_Static_assert(sizeof(struct efex_param_area) == EFEX_PARAM_AREA_SIZE, "eFEX parameter area must be 4 KiB");

/* Key encoding. */
#define EFEX_PARAM_APPLIED (1U << 31)
#define EFEX_PARAM_OUTPUT  (1U << 30)
#define EFEX_PARAM_FLAGS   (EFEX_PARAM_APPLIED | EFEX_PARAM_OUTPUT)
#define EFEX_PARAM_KEY(group, id, index) \
	((((uint32_t)(group) & 0x3fU) << 24) | (((uint32_t)(id) & 0xffffU) << 8) | ((uint32_t)(index) & 0xffU))

/* Groups. */
#define EFEX_PARAM_GROUP_UART	  0x01U
#define EFEX_PARAM_GROUP_PMU_TWI  0x02U
#define EFEX_PARAM_GROUP_PMU_RAIL 0x03U
#define EFEX_PARAM_GROUP_DRAM	  0x04U
#define EFEX_PARAM_GROUP_PSRAM	  0x05U
#define EFEX_PARAM_GROUP_APP	  0x3fU

/*
 * Bus fields shared by the UART and PMU TWI groups.  PIN0/PIN1 are TX/RX for
 * UART and SCL/SDA for TWI, encoded with GPIO_PIN(port, n).  GPIO_BANK0 is the
 * first port of the pin controller at GPIO_BASE (GPIO_PORTL for R_PIO), so the
 * driver bank is port - GPIO_BANK0.
 */
#define EFEX_PARAM_BUS_BASE	  0x00U
#define EFEX_PARAM_BUS_ID	  0x01U
#define EFEX_PARAM_BUS_RATE	  0x02U /* UART baud rate, TWI speed in Hz */
#define EFEX_PARAM_BUS_PIN0	  0x03U
#define EFEX_PARAM_BUS_MUX0	  0x04U
#define EFEX_PARAM_BUS_PIN1	  0x05U
#define EFEX_PARAM_BUS_MUX1	  0x06U
#define EFEX_PARAM_BUS_GPIO_BASE  0x07U
#define EFEX_PARAM_BUS_GPIO_BANK0 0x08U
#define EFEX_PARAM_BUS_GATE_REG	  0x09U
#define EFEX_PARAM_BUS_GATE_BIT	  0x0aU
#define EFEX_PARAM_BUS_RST_REG	  0x0bU
#define EFEX_PARAM_BUS_RST_BIT	  0x0cU
#define EFEX_PARAM_BUS_PARENT_CLK 0x0dU
#define EFEX_PARAM_UART_PARITY	  0x0eU
#define EFEX_PARAM_UART_STOP	  0x0fU
#define EFEX_PARAM_UART_DLEN	  0x10U

#define EFEX_PARAM_UART(field)	  EFEX_PARAM_KEY(EFEX_PARAM_GROUP_UART, field, 0)
#define EFEX_PARAM_PMU_TWI(field) EFEX_PARAM_KEY(EFEX_PARAM_GROUP_PMU_TWI, field, 0)

/* PMU rail voltage in mV: id = PMU number, index = rail number, both per app. */
#define EFEX_PARAM_PMU_RAIL(pmu, rail) EFEX_PARAM_KEY(EFEX_PARAM_GROUP_PMU_RAIL, pmu, rail)
#define EFEX_PARAM_RAIL_MAX	       16U

/* Application-defined value n (24 bits: id and index together). */
#define EFEX_PARAM_APP(n) ((EFEX_PARAM_GROUP_APP << 24) | ((uint32_t)(n) & 0xffffffU))

/* DRAM and PSRAM fields. */
#define EFEX_PARAM_MEM_PARA	  0x00U /* index = parameter word */
#define EFEX_PARAM_MEM_PARA_COUNT 0x01U
#define EFEX_PARAM_MEM_BASE	  0x02U
#define EFEX_PARAM_MEM_SIZE	  0x03U
#define EFEX_PARAM_MEM_SIZE_MB	  0x10U /* output */
#define EFEX_PARAM_MEM_INIT_OK	  0x11U /* output */

#define EFEX_PARAM_DRAM(field, index)  EFEX_PARAM_KEY(EFEX_PARAM_GROUP_DRAM, field, index)
#define EFEX_PARAM_PSRAM(field, index) EFEX_PARAM_KEY(EFEX_PARAM_GROUP_PSRAM, field, index)

extern struct efex_param_area efex_param;

/**
 * @brief Everything efex_param_load() may override; NULL members are skipped.
 *
 * rail_mv is the application's [pmu][rail] table of defaults in mV, which it
 * then passes to the PMU set_vol calls; app holds application-defined values
 * indexed by EFEX_PARAM_APP(n).
 */
struct efex_param_targets {
	sunxi_serial_t *uart;
	sunxi_i2c_t *i2c;
	sunxi_dram_t *dram;
	sunxi_psram_t *psram;
	int (*rail_mv)[EFEX_PARAM_RAIL_MAX];
	size_t pmu_count;
	uint32_t *app;
	size_t app_count;
};

/**
 * @brief Read the host table once and apply it to @p targets.
 *
 * Call it once at the start of main(), after the defaults are in place and
 * before any driver init.  Entries that were applied get EFEX_PARAM_APPLIED;
 * entries without a matching target are left unmarked.  A rejected table
 * (bad header or checksum) is reset to empty, every default is kept, and the
 * final status reads EFEX_PARAM_STATUS_BAD.
 *
 * @return true if the host table was accepted.
 */
bool efex_param_load(const struct efex_param_targets *targets);

/**
 * @brief Report a result for the host.
 *
 * Updates an existing entry in place or appends one, marking it
 * EFEX_PARAM_OUTPUT.
 *
 * @return 0 on success, -1 when the table is full.
 */
int efex_param_put(uint32_t key, uint32_t value);

/** @brief Report @p count words as keys EFEX_PARAM_KEY(group, id, 0..count - 1). */
int efex_param_put_array(uint32_t group, uint32_t id, const uint32_t *values, size_t count);

/** @brief Report DRAM/PSRAM parameters (after training), size and result. */
void efex_param_report_dram(const sunxi_dram_t *dram, uint32_t size_mb);
void efex_param_report_psram(const sunxi_psram_t *psram, uint32_t size_mb);

/** @brief Store the final status, @p ret and checksum; called by the eFEX entry after main(). */
void efex_param_finish(int ret);

#endif
