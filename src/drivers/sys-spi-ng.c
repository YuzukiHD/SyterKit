/* SPDX-License-Identifier: Apache-2.0 */

#define LOG_LEVEL_DEFAULT LOG_LEVEL_DEBUG

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <log.h>

#include <sys-spi.h>

/* DMA Handler */
static __attribute__((section(".data"))) sunxi_dma_set_t spi_rx_dma;
static uint32_t spi_dma_handler = 0;

static inline void sunxi_spi_soft_reset(sunxi_spi_t *spi) {
    sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
    spi_reg->gc |= SPI_GC_SRST;
}

static inline void sunxi_spi_enable_bus(sunxi_spi_t *spi) {
    sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
    spi_reg->gc |= SPI_GC_EN;
}

static inline void sunxi_spi_disable_bus(sunxi_spi_t *spi) {
    sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
    spi_reg->gc &= ~SPI_GC_EN;
}

static inline void sunxi_spi_set_cs(sunxi_spi_t *spi, uint8_t cs) {
    sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
    spi_reg->tc &= ~SPI_TC_SS_MASK;         /* SS-chip select, clear two bits */
    spi_reg->tc |= cs << SPI_TC_SS_BIT_POS; /* set chip select */
}

static inline void sunxi_spi_set_master(sunxi_spi_t *spi) {
    sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
    spi_reg->gc |= SPI_GC_MODE;
}

static inline void sunxi_spi_start_xfer(sunxi_spi_t *spi) {
    sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
    spi_reg->tc |= SPI_TC_XCH;
}

static inline void sunxi_spi_enable_transmit_pause(sunxi_spi_t *spi) {
    sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
    spi_reg->gc |= SPI_GC_TP_EN;
}

static inline void sunxi_spi_set_ss_owner(sunxi_spi_t *spi, u32 on_off) {
    sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
    on_off &= 0x1;
    if (on_off)
        spi_reg->tc |= SPI_TC_SS_OWNER;
    else
        spi_reg->tc &= ~SPI_TC_SS_OWNER;
}

static inline uint32_t sunxi_spi_query_txfifo(sunxi_spi_t *spi) {
    sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
    uint32_t reg_val = (SPI_FIFO_STA_TX_CNT & spi_reg->fifo_sta);
    reg_val >>= SPI_TXCNT_BIT_POS;
    return reg_val;
}

static inline uint32_t sunxi_spi_query_rxfifo(sunxi_spi_t *spi) {
    sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
    uint32_t reg_val = (SPI_FIFO_STA_RX_CNT & spi_reg->fifo_sta);
    reg_val >>= SPI_RXCNT_BIT_POS;
    return reg_val;
}

static inline void sunxi_spi_disable_irq(sunxi_spi_t *spi, uint32_t bitmap) {
    sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
    bitmap &= SPI_INTEN_MASK;
    spi_reg->int_ctl &= ~bitmap;
}

static inline void sunxi_spi_clr_irq_pending(sunxi_spi_t *spi, uint32_t pending_bit) {
    sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
    pending_bit &= SPI_INT_STA_MASK;
    spi_reg->int_sta = pending_bit;
}

static inline uint32_t sunxi_spi_query_irq_pending(sunxi_spi_t *spi) {
    sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
    return (SPI_INT_STA_MASK & spi_reg->int_sta);
}

static inline void sunxi_spi_set_ss_level(sunxi_spi_t *spi, uint32_t high_low) {
    sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
    high_low &= 0x1;
    if (high_low)
        spi_reg->tc |= SPI_TC_SS_LEVEL;
    else
        spi_reg->tc &= ~SPI_TC_SS_LEVEL;
}

static inline void sunxi_spi_dma_disable(sunxi_spi_t *spi) {
    sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
    spi_reg->fifo_ctl &= ~(SPI_FIFO_CTL_TX_DRQEN | SPI_FIFO_CTL_RX_DRQEN);
}

/* reset fifo */
static void sunxi_spi_reset_fifo(sunxi_spi_t *spi) {
    sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
    uint32_t reg_val = spi_reg->fifo_ctl;

    reg_val |= (SPI_FIFO_CTL_RX_RST | SPI_FIFO_CTL_TX_RST);
    /* Set the trigger level of RxFIFO/TxFIFO. */
    reg_val &= ~(SPI_FIFO_CTL_RX_LEVEL | SPI_FIFO_CTL_TX_LEVEL);
    reg_val |= (0x20 << 16) | 0x20;
    spi_reg->fifo_ctl = reg_val;
}

static uint32_t sunxi_spi_read_rx_fifo(sunxi_spi_t *spi, uint8_t *buf, uint32_t len) {
    sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
    while ((len -= SPI_FIFO_CTL_SHIFT % SPI_FIFO_CTL_SHIFT) == 0) {
        while (sunxi_spi_query_rxfifo(spi) < SPI_FIFO_CTL_SHIFT) {
        };
        *(buf += SPI_FIFO_CTL_SHIFT) = spi_reg->rxdata;
    }

    while (len-- > 0) {
        while (sunxi_spi_query_rxfifo(spi) < 1)
            ;
        *buf++ = read8((virtual_addr_t) &spi_reg->rxdata);
    }
    return len;
}

static void sunxi_spi_write_tx_fifo(sunxi_spi_t *spi, uint8_t *buf, uint32_t len) {
    sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
    uint32_t tx_len = len;
    uint8_t *tx_buf = buf;

    while ((len -= SPI_FIFO_CTL_SHIFT % SPI_FIFO_CTL_SHIFT) == 0) {
        while (sunxi_spi_query_txfifo(spi) > MAX_FIFU - SPI_FIFO_CTL_SHIFT) {
            udelay(10);
        };
        spi_reg->txdata = *(buf += SPI_FIFO_CTL_SHIFT);
    }

    while (len-- > 0) {
        while (sunxi_spi_query_txfifo(spi) >= MAX_FIFU) {
            udelay(10);
        };
        write8((virtual_addr_t) &spi_reg->txdata, *buf++);
    }
}

static void sunxi_spi_read_by_dma(sunxi_spi_t *spi, uint8_t *buf, uint32_t len) {
    sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
    uint8_t ret = 0x0;

    memset(buf, 0x0, len);

    spi_reg->fifo_ctl |= SPI_FIFO_CTL_RX_DRQEN;
    ret = sunxi_dma_start(spi_dma_handler, (uint32_t) &spi_reg->rxdata, (uint32_t) buf, len);
    if (ret) {
        printk_warning("SPI: DMA transfer failed\n");
    }

    while (sunxi_dma_querystatus(spi_dma_handler))
        ;
}

static uint32_t sunxi_spi_set_clk(sunxi_spi_t *spi, u32 spi_clk, u32 mclk, u32 cdr2) {
    sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;

    uint32_t reg = 0;
    uint32_t div = 1;
    uint32_t src_clk = mclk;
    uint32_t freq = spi->parent_clk_reg.parent_clk;

    if (spi_clk != spi->parent_clk_reg.parent_clk) {
        /* CDR2 */
        if (cdr2) {
            div = mclk / (spi_clk * 2) - 1;
            reg |= SPI_CLK_CTL_CDR2(div) | SPI_CLK_CTL_DRS;
            printk_debug("SPI: CDR2 - n = %lu\n", div);
            freq = mclk / (2 * ((div + 1)));
        } else { /* CDR1 */
            while (src_clk > spi_clk) {
                div++;
                src_clk >>= 1;
            }
            reg |= SPI_CLK_CTL_CDR1(div);
            printk_debug("SPI: CDR1 - n = %lu\n", div);
            freq = src_clk;
        }
    }

    printk_debug("SPI: clock div=%u \n", div);
    printk_debug("SPI: set clock asked=%dMHz actual=%dMHz mclk=%dMHz\n",
                 spi_clk / 1000000, freq / 1000000, mclk / 1000000);

    spi_reg->clk_ctl = reg;

    spi->spi_clk.spi_clock_freq = freq;

    return freq;
}

static int sunxi_spi_dma_init(sunxi_spi_t *spi) {
    sunxi_dma_init(spi->dma_handle);

    spi_dma_handler = sunxi_dma_request(DMAC_DMATYPE_NORMAL);

    if (spi_dma_handler == 0) {
        printk_error("SPI: DMA channel request failed\n");
        return -1;
    }

    /* config spi rx dma */
    spi_rx_dma.loop_mode = 0;
    spi_rx_dma.wait_cyc = 0x8;
    spi_rx_dma.data_block_size = 1 * 32 / 8;

    spi_rx_dma.channel_cfg.src_drq_type = DMAC_CFG_TYPE_SPI0; /* SPI0 */
    spi_rx_dma.channel_cfg.src_addr_mode = DMAC_CFG_SRC_ADDR_TYPE_IO_MODE;
    spi_rx_dma.channel_cfg.src_burst_length = DMAC_CFG_SRC_8_BURST;
    spi_rx_dma.channel_cfg.src_data_width = DMAC_CFG_SRC_DATA_WIDTH_32BIT;

    spi_rx_dma.channel_cfg.dst_drq_type = DMAC_CFG_TYPE_DRAM; /* DRAM */
    spi_rx_dma.channel_cfg.dst_addr_mode = DMAC_CFG_DEST_ADDR_TYPE_LINEAR_MODE;
    spi_rx_dma.channel_cfg.dst_burst_length = DMAC_CFG_DEST_8_BURST;
    spi_rx_dma.channel_cfg.dst_data_width = DMAC_CFG_DEST_DATA_WIDTH_32BIT;

    /* init DMA settings */
    sunxi_dma_install_int(spi_dma_handler, NULL);
    sunxi_dma_enable_int(spi_dma_handler);
    sunxi_dma_setting(spi_dma_handler, &spi_rx_dma);

    return 0;
}

static int sunxi_spi_dma_deinit(sunxi_spi_t *spi) {
    sunxi_dma_disable_int(spi_dma_handler);
    return 0;
}

static int sunxi_spi_get_clk(sunxi_spi_t *spi) {
    u32 reg_val = 0;
    u32 src = 0, clk = 0, sclk_freq = 0;
    u32 n, m;

    reg_val = read32(spi->spi_clk.spi_clock_cfg_base);
    src = (reg_val >> 24) & 0x7;
    n = (reg_val >> spi->spi_clk.spi_clock_factor_n_offset) & 0x3;
    m = ((reg_val >> 0) & 0xf) + 1;

    switch (src) {
        case 0:
            clk = 24000000;
            break;
        case 1:
        case 2:
            clk = spi->parent_clk_reg.parent_clk;
            break;
        default:
            clk = 0;
            break;
    }
    sclk_freq = clk / (1 << n) / m;
    printk_trace("SPI: sclk_freq= %d Hz, reg_val: 0x%08x , n=%d, m=%d\n", sclk_freq, reg_val, n, m);
    return sclk_freq;
}

static void sunxi_spi_clk_init(sunxi_spi_t *spi) {
    uint32_t div, source_clk, mod_clk, n, m;
    uint32_t reg_val;

    /* close gate */
    clrbits_le32(spi->parent_clk_reg.gate_reg_base, BIT(spi->parent_clk_reg.gate_reg_offset));

    source_clk = spi->parent_clk_reg.parent_clk;
    mod_clk = spi->clk_rate;

    div = (source_clk + mod_clk - 1) / mod_clk;
    div = div == 0 ? 1 : div;
    if (div > 128) {
        m = 1;
        n = 0;
        return;
    } else if (div > 64) {
        n = 3;
        m = div >> 3;
    } else if (div > 32) {
        n = 2;
        m = div >> 2;
    } else if (div > 16) {
        n = 1;
        m = div >> 1;
    } else {
        n = 0;
        m = div;
    }

    /* set m factor, factor_m = m -1 */
    m -= 1;

    reg_val = BIT(31) | (spi->spi_clk.spi_clock_source << 24) | (n << spi->spi_clk.spi_clock_factor_n_offset) | m;

    /* enable spi clock */
    write32(spi->spi_clk.spi_clock_cfg_base, reg_val);

    /* SPI Reset */
    clrbits_le32(spi->parent_clk_reg.rst_reg_base, BIT(spi->parent_clk_reg.rst_reg_offset));
    udelay(1);
    setbits_le32(spi->parent_clk_reg.rst_reg_base, BIT(spi->parent_clk_reg.rst_reg_offset));

    /* SPI gating */
    setbits_le32(spi->parent_clk_reg.gate_reg_base, BIT(spi->parent_clk_reg.gate_reg_offset));

    sunxi_spi_get_clk(spi);
}

static void sunxi_spi_clk_deinit(sunxi_spi_t *spi) {
    /* SPI Assert */
    clrbits_le32(spi->parent_clk_reg.rst_reg_base, BIT(spi->parent_clk_reg.rst_reg_offset));

    /* SPI Close gating */
    clrbits_le32(spi->parent_clk_reg.gate_reg_base, BIT(spi->parent_clk_reg.gate_reg_offset));
}

static void sunxi_spi_config_transer_control(sunxi_spi_t *spi) {
    sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;

    uint32_t reg_val = spi_reg->tc;

    if (spi->spi_clk.spi_clock_freq > 100000000) {
        reg_val &= ~(SPI_TC_SDC | SPI_TC_SDM);
        reg_val |= SPI_TC_SDC;
    } else if (spi->spi_clk.spi_clock_freq <= 24000000) {
        reg_val &= ~(SPI_TC_SDC | SPI_TC_SDM);
        reg_val |= SPI_TC_SDM;
    } else {
        reg_val &= ~(SPI_TC_SDC | SPI_TC_SDM);
    }
    reg_val |= SPI_TC_DHB | SPI_TC_SS_LEVEL | SPI_TC_SPOL;

    spi_reg->tc = reg_val;
}

static void sunxi_spi_set_io_mode(sunxi_spi_t *spi, spi_io_mode_t mode) {
    sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;

    spi_reg->bcc &= ~(SPI_BCC_QUAD_MODE | SPI_BCC_DUAL_MODE);
    switch (mode) {
        case SPI_IO_DUAL_RX:
            spi_reg->bcc |= SPI_BCC_DUAL_MODE;
            break;
        case SPI_IO_QUAD_RX:
        case SPI_IO_QUAD_IO:
            spi_reg->bcc |= SPI_BCC_QUAD_MODE;
            break;
        case SPI_IO_SINGLE:
        default:
            break;
    }
}

static void sunxi_spi_set_counters(sunxi_spi_t *spi, int txlen, int rxlen, int stxlen, int dummylen) {
    sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
    uint32_t val;

    val = spi_reg->burst_cnt;
    val &= ~SPI_BC_CNT_MASK;
    val |= (SPI_BC_CNT_MASK & (txlen + rxlen + dummylen));
    spi_reg->burst_cnt = val;

    val = spi_reg->transmit_cnt;
    val &= ~SPI_TC_CNT_MASK;
    val |= (SPI_TC_CNT_MASK & txlen);
    spi_reg->transmit_cnt = val;

    val = spi_reg->bcc;
    val &= ~SPI_BCC_STC_MASK;
    val |= (SPI_BCC_STC_MASK & stxlen);
    val &= ~SPI_BCC_DBC_MASK;
    val |= (dummylen << SPI_BCC_DBC_POS);
    spi_reg->bcc = val;
}

static void sunxi_spi_bus_init(sunxi_spi_t *spi) {
    sunxi_spi_soft_reset(spi);

    sunxi_spi_enable_bus(spi);

    sunxi_spi_set_cs(spi, 0);

    sunxi_spi_set_master(spi);

    sunxi_spi_set_clk(spi, spi->clk_rate, spi->parent_clk_reg.parent_clk, 1);

    sunxi_spi_config_transer_control(spi);

    sunxi_spi_set_ss_level(spi, 1);

    sunxi_spi_enable_transmit_pause(spi);

    sunxi_spi_set_ss_owner(spi, 0);

    sunxi_spi_reset_fifo(spi);
}

static void sunxi_spi_gpio_init(sunxi_spi_t *spi) {
    /* Config SPI pins */
    sunxi_gpio_init(spi->gpio.gpio_cs.pin, spi->gpio.gpio_cs.mux);
    sunxi_gpio_init(spi->gpio.gpio_sck.pin, spi->gpio.gpio_sck.mux);
    sunxi_gpio_init(spi->gpio.gpio_mosi.pin, spi->gpio.gpio_mosi.mux);
    sunxi_gpio_init(spi->gpio.gpio_miso.pin, spi->gpio.gpio_miso.mux);
    sunxi_gpio_init(spi->gpio.gpio_wp.pin, spi->gpio.gpio_wp.mux);
    sunxi_gpio_init(spi->gpio.gpio_hold.pin, spi->gpio.gpio_hold.mux);

    /* Floating by default */
    sunxi_gpio_set_pull(spi->gpio.gpio_wp.pin, GPIO_PULL_UP);
    sunxi_gpio_set_pull(spi->gpio.gpio_hold.pin, GPIO_PULL_UP);
}

int sunxi_spi_init(sunxi_spi_t *spi) {
    /* if set dma handle, we using dma mode */
    if (spi->dma_handle != NULL) {
        sunxi_spi_dma_init(spi);
    }

    sunxi_spi_gpio_init(spi);

    sunxi_spi_clk_init(spi);

    sunxi_spi_bus_init(spi);

    return 0;
}

void sunxi_spi_disable(sunxi_spi_t *spi) {
    sunxi_spi_disable_bus(spi);
    sunxi_spi_dma_deinit(spi);
    sunxi_spi_clk_deinit(spi);
}

int sunxi_spi_update_clk(sunxi_spi_t *spi, uint32_t clk) {
    spi->clk_rate = clk;
    sunxi_spi_clk_init(spi);
    sunxi_spi_bus_init(spi);
    sunxi_spi_config_transer_control(spi);
    return 0;
}

int sunxi_spi_transfer(sunxi_spi_t *spi, spi_io_mode_t mode, void *txbuf, uint32_t txlen, void *rxbuf, uint32_t rxlen) {
    sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
    uint32_t stxlen, fcr;

    printk_trace("SPI: tsfr mode=%u tx=%u rx=%u\n", mode, txlen, rxlen);

    sunxi_spi_disable_irq(spi, SPI_INT_STA_PENDING_BIT);
    sunxi_spi_clr_irq_pending(spi, SPI_INT_STA_PENDING_BIT);

    sunxi_spi_set_io_mode(spi, mode);

    switch (mode) {
        case SPI_IO_QUAD_IO:
            stxlen = 1;// Only opcode
            break;
        case SPI_IO_DUAL_RX:
        case SPI_IO_QUAD_RX:
            stxlen = txlen;// Only tx data
            break;
        case SPI_IO_SINGLE:
        default:
            stxlen = txlen + rxlen;// both tx and rx data
            break;
    };

    sunxi_spi_set_counters(spi, txlen, rxlen, stxlen, 0);
    sunxi_spi_reset_fifo(spi);
    sunxi_spi_start_xfer(spi);

    if (txbuf && txlen) {
        sunxi_spi_write_tx_fifo(spi, txbuf, txlen);
    }

    if (rxbuf && rxlen) {
        if (rxlen > 64) {
            sunxi_spi_read_by_dma(spi, rxbuf, rxlen);
        } else {
            sunxi_spi_read_rx_fifo(spi, rxbuf, rxlen);
        }
    }

    if (sunxi_spi_query_irq_pending(spi) & SPI_INT_STA_ERR) {
        printk_warning("SPI: int sta err\n");
    }

    while (!(sunxi_spi_query_irq_pending(spi) & SPI_INT_STA_TC))
        ;

    sunxi_spi_dma_disable(spi);

    if (spi_reg->burst_cnt == 0) {
        if (spi_reg->tc & SPI_TC_XCH) {
            printk_warning("SPI: XCH Control failed\n");
        }
    } else {
        printk_warning("SPI: MBC error\n");
    }

    sunxi_spi_clr_irq_pending(spi, SPI_INT_STA_PENDING_BIT);

    printk_trace("SPI: ISR=0x%x\n", spi_reg->int_sta);

    return rxlen + txlen;
}