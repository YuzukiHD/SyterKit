/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_SPIF_H__
#define __DRIVERS_SPIF_H__

#include <stdint.h>
#include <types.h>

#include <drivers/clk/clk.h>
#include <drivers/gpio/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SUNXI_SPIF_COMPATIBLE "allwinner,sunxi-spif"

#define SUNXI_SPIF_DEFAULT_FREQUENCY 20000000U
#define SUNXI_SPIF_MAX_FREQUENCY     150000000U
#define SUNXI_SPIF_MIN_FREQUENCY     187500U
#define SUNXI_SPIF_CACHELINE_SIZE    64U
#define SUNXI_SPIF_SAMPLE_DEFAULT    0xaaaaffffU

#define SPIF_CFG_SPEED_HZ     (1U << 0)
#define SPIF_CFG_RX_DTR	      (1U << 1)
#define SPIF_CFG_TX_DTR	      (1U << 2)
#define SPIF_CFG_SAMPLE_MODE  (1U << 3)
#define SPIF_CFG_SAMPLE_DELAY (1U << 4)

#define SPIF_RX_SLOW  (1U << 11)
#define SPIF_RX_DUAL  (1U << 12)
#define SPIF_RX_QUAD  (1U << 13)
#define SPIF_RX_OCTAL (1U << 15)
#define SPIF_TX_BYTE  (1U << 8)
#define SPIF_TX_QUAD  (1U << 10)
#define SPIF_TX_OCTAL (1U << 14)
#define SPIF_DTR_MODE (1U << 16)
#define SPIF_IO_MODE  (1U << 17)

struct spif_cfg {
	u32 valid;
	u32 speed_hz;
	u32 rx_dtr_en;
	u32 tx_dtr_en;
	u32 sample_mode;
	u32 sample_delay;
};

enum sunxi_spif_clock_layout {
	SUNXI_SPIF_CLOCK_LAYOUT_NM = 0,
	SUNXI_SPIF_CLOCK_LAYOUT_DIV2 = 1,
};

typedef struct sunxi_spif {
	int dt_node;
	uintptr_t base;
	uint8_t id;
	uint8_t chip_select;
	uint8_t initialized;
	uint8_t dtr_active;

	uint32_t bus_freq;
	uint32_t speed_hz;
	uint32_t actual_speed_hz;
	uint32_t min_speed_hz;
	uint32_t max_speed_hz;
	uint32_t mode;
	uint32_t sample_delay;
	uint32_t sample_mode;
	uint32_t rx_dtr_en;
	uint32_t tx_dtr_en;

	uintptr_t clock_reg;
	uint32_t clock_source;
	uint32_t clock_parent_hz;
	uint32_t clock_n_offset;
	uint32_t clock_layout;
	sunxi_clk_t clk;

	gpio_mux_t gpio_cs;
	gpio_mux_t gpio_sck;
	gpio_mux_t gpio_mosi;
	gpio_mux_t gpio_miso;
	gpio_mux_t gpio_wp;
	gpio_mux_t gpio_hold;
} sunxi_spif_t;

enum spi_mem_data_dir {
	SPI_MEM_NO_DATA,
	SPI_MEM_DATA_IN,
	SPI_MEM_DATA_OUT,
};

struct spi_mem_op {
	struct {
		u8 nbytes;
		u8 buswidth;
		u8 dtr : 1;
		u16 opcode;
	} cmd;

	struct {
		u8 nbytes;
		u8 buswidth;
		u8 dtr : 1;
		u64 val;
	} addr;

	struct {
		u8 buswidth;
		const void *val;
	} mode;

	struct {
		u8 nbytes;
		u8 buswidth;
		u8 dtr : 1;
	} dummy;

	struct {
		u8 buswidth;
		u8 dtr : 1;
		enum spi_mem_data_dir dir;
		u32 nbytes;
		union {
			void *in;
			const void *out;
		} buf;
	} data;
};

enum spi_mem_buswidth {
	SPI_MEM_BUSWIDTH_1 = 1,
	SPI_MEM_BUSWIDTH_2 = 2,
	SPI_MEM_BUSWIDTH_4 = 4,
	SPI_MEM_BUSWIDTH_8 = 8,
};

/** @brief Initialize a SPIF instance after sunxi_spif_dt_read_* filled it. */
int sunxi_spif_init(sunxi_spif_t *spif);

/** @brief Disable the SPIF clock and controller. */
void sunxi_spif_disable(sunxi_spif_t *spif);

/** @brief Select a hardware chip-select line. */
int sunxi_spif_select(sunxi_spif_t *spif, uint8_t chip_select);

/** @brief Reconfigure the SPIF serial clock. */
int sunxi_spif_update_clk(sunxi_spif_t *spif, uint32_t speed_hz);

/** @brief Apply a subset of the SPIF runtime configuration. */
int sunxi_spif_set_config(sunxi_spif_t *spif, const struct spif_cfg *cfg);

/** @brief Execute one SPI operation synchronously. */
int sunxi_spif_exec_op(sunxi_spif_t *spif, const struct spi_mem_op *op);

#ifdef __cplusplus
}
#endif

#endif /* __DRIVERS_SPIF_H__ */
