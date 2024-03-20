/* SPDX-License-Identifier: Apache-2.0 */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <common.h>
#include <log.h>

#include <pmu/axp.h>

static uint32_t AXP2202_RUNTIME_ADDR = 0x0;

/* clang-format off */
static axp_contrl_info axp_ctrl_tbl[] = {
	{ "dcdc1", 500, 1540, AXP2202_DC1OUT_VOL, 0x7f, AXP2202_OUTPUT_CTL0, 0, 0,
	{ {500, 1200, 10}, {1220, 1540, 20}, } },

	{ "dcdc2", 500, 3400, AXP2202_DC2OUT_VOL, 0x7f, AXP2202_OUTPUT_CTL0, 1, 0,
	{ {500, 1200, 10}, {1220, 1540, 20}, {1600, 3400, 100},} },

	{ "dcdc3", 500, 1840, AXP2202_DC3OUT_VOL, 0x7f, AXP2202_OUTPUT_CTL0, 2, 0,
	{ {500, 1200, 10}, {1220, 1840, 20}, } },

	{ "dcdc4", 1000, 3400, AXP2202_DC4OUT_VOL, 0x7f, AXP2202_OUTPUT_CTL0, 3, 0,
	{ {1000, 3400, 100}, } },

	{ "aldo1", 500, 3500, AXP2202_ALDO1OUT_VOL, 0x1f, AXP2202_OUTPUT_CTL2, 0, 0,
	{ {500, 3500, 100}, } },

	{ "aldo2", 500, 3500, AXP2202_ALDO2OUT_VOL, 0x1f, AXP2202_OUTPUT_CTL2, 1, 0,
	{ {500, 3500, 100}, } },

	{ "aldo3", 500, 3500, AXP2202_ALDO3OUT_VOL, 0x1f, AXP2202_OUTPUT_CTL2, 2, 0,
	{ {500, 3500, 100}, } },

	{ "aldo4", 500, 3500, AXP2202_ALDO4OUT_VOL, 0x1f, AXP2202_OUTPUT_CTL2, 3, 0,
	{ {500, 3500, 100}, } },

	{ "bldo1", 500, 3500, AXP2202_BLDO1OUT_VOL, 0x1f, AXP2202_OUTPUT_CTL2, 4, 0,
	{ {500, 3500, 100}, } },

	{ "bldo2", 500, 3500, AXP2202_BLDO2OUT_VOL, 0x1f, AXP2202_OUTPUT_CTL2, 5, 0,
	{ {500, 3500, 100}, } },

	{ "bldo3", 500, 3500, AXP2202_BLDO3OUT_VOL, 0x1f, AXP2202_OUTPUT_CTL2, 6, 0,
	{ {500, 3500, 100}, } },

	{ "bldo4", 500, 3500, AXP2202_BLDO4OUT_VOL, 0x1f, AXP2202_OUTPUT_CTL2, 7, 0,
	{ {500, 3500, 100}, } },

	{ "cldo1", 500, 3500, AXP2202_CLDO1OUT_VOL, 0x1f, AXP2202_OUTPUT_CTL3, 0, 0,
	{ {500, 3500, 100}, } },

	{ "cldo2", 500, 3500, AXP2202_CLDO2OUT_VOL, 0x1f, AXP2202_OUTPUT_CTL3, 1, 0,
	{ {500, 3500, 100}, } },

	{ "cldo3", 500, 3500, AXP2202_CLDO3OUT_VOL, 0x1f, AXP2202_OUTPUT_CTL3, 2, 0,
	{ {500, 3500, 100}, } },

	{ "cldo4", 500, 3500, AXP2202_CLDO4OUT_VOL, 0x1f, AXP2202_OUTPUT_CTL3, 3, 0,
	{ {500, 3500, 100}, } },

	{ "cpusldo", 500, 1400, AXP2202_CPUSLDO_VOL, 0x1f, AXP2202_OUTPUT_CTL3, 4, 0,
	{ {500, 1400, 50}, } },
};
/* clang-format on */

/**
 * Get control information from the table based on the given name.
 *
 * @param name The name of the control information to retrieve.
 * @return A pointer to the axp_contrl_info structure corresponding to the given name,
 *         or NULL if the name is not found in the table.
 */
static axp_contrl_info *get_ctrl_info_from_tbl(char *name) {
    int i = 0;
    int size = ARRAY_SIZE(axp_ctrl_tbl);
    for (i = 0; i < size; i++) {
        if (!strncmp(name, axp_ctrl_tbl[i].name, strlen(axp_ctrl_tbl[i].name))) {
            break;
        }
    }
    if (i >= size) {
        return NULL;
    }
    return (axp_ctrl_tbl + i);
}

int pmu_axp2202_init(sunxi_i2c_t *i2c_dev) {
    uint8_t axp_val;
    int ret;

    if (!i2c_dev->status) {
        printk(LOG_LEVEL_WARNING, "PMU: I2C not init\n");
        return -1;
    }

    /* Try to probe AXP717B */
    if (sunxi_i2c_read(i2c_dev, AXP2202_B_RUNTIME_ADDR, AXP2202_CHIP_ID_EXT, &axp_val)) {
        /* AXP717B probe fail, Try to probe AXP717C */
        if (sunxi_i2c_read(i2c_dev, AXP2202_C_RUNTIME_ADDR, AXP2202_CHIP_ID_EXT, &axp_val)) {
            /* AXP717C probe fail */
            printk(LOG_LEVEL_WARNING, "PMU: AXP2202 PMU Read error\n");
            return -1;
        } else {
            AXP2202_RUNTIME_ADDR = AXP2202_C_RUNTIME_ADDR;
        }
    } else {
        AXP2202_RUNTIME_ADDR = AXP2202_B_RUNTIME_ADDR;
    }

    if (axp_val != 0x02) {
        printk(LOG_LEVEL_WARNING, "PMU: AXP PMU Check error\n");
        return -1;
    } else {
        printk(LOG_LEVEL_INFO, "PMU: Found AXP717 PMU, Addr 0x%02x\n", AXP2202_RUNTIME_ADDR);
    }

    /* limit run current to 2A */
    axp_val = 0x26;
    sunxi_i2c_write(i2c_dev, AXP2202_RUNTIME_ADDR, AXP2202_VBUS_CUR_SET, axp_val);

    /* set adc channel0 enable */
    sunxi_i2c_read(i2c_dev, AXP2202_RUNTIME_ADDR, AXP2202_ADC_CH0, &axp_val);
    axp_val |= 0x33;
    sunxi_i2c_write(i2c_dev, AXP2202_RUNTIME_ADDR, AXP2202_ADC_CH0, axp_val);

    /*pmu set vsys min*/
    sunxi_i2c_read(i2c_dev, AXP2202_RUNTIME_ADDR, AXP2202_VSYS_MIN, &axp_val);
    axp_val = 0x06;
    sunxi_i2c_write(i2c_dev, AXP2202_RUNTIME_ADDR, AXP2202_VSYS_MIN, axp_val);

    /*pmu dcdc1 uvp disable */
    sunxi_i2c_read(i2c_dev, AXP2202_RUNTIME_ADDR, AXP2202_DCDC_PWEOFF_EN, &axp_val);
    axp_val &= ~(1 << 0);
    sunxi_i2c_write(i2c_dev, AXP2202_RUNTIME_ADDR, AXP2202_DCDC_PWEOFF_EN, axp_val);

    sunxi_i2c_read(i2c_dev, AXP2202_RUNTIME_ADDR, AXP2202_CHIP_VER_EXT, &axp_val);

    if (axp_val) {
        sunxi_i2c_read(i2c_dev, AXP2202_RUNTIME_ADDR, AXP2202_MODULE_EN, &axp_val);
        axp_val |= 0x10;
        sunxi_i2c_write(i2c_dev, AXP2202_RUNTIME_ADDR, AXP2202_MODULE_EN, axp_val);
    } else {
        sunxi_i2c_read(i2c_dev, AXP2202_RUNTIME_ADDR, AXP2202_MODULE_EN, &axp_val);
        axp_val &= 0xEF;
        sunxi_i2c_write(i2c_dev, AXP2202_RUNTIME_ADDR, AXP2202_MODULE_EN, axp_val);
    }

    return 0;
}

int pmu_axp2202_set_vol(sunxi_i2c_t *i2c_dev, char *name, int set_vol, int onoff) {
    return axp_set_vol(i2c_dev, name, set_vol, onoff, axp_ctrl_tbl, ARRAY_SIZE(axp_ctrl_tbl), AXP2202_RUNTIME_ADDR);
}

int pmu_axp2202_get_vol(sunxi_i2c_t *i2c_dev, char *name) {
    return axp_get_vol(i2c_dev, name, axp_ctrl_tbl, ARRAY_SIZE(axp_ctrl_tbl), AXP2202_RUNTIME_ADDR);
}

void pmu_axp2202_dump(sunxi_i2c_t *i2c_dev) {
    printk(LOG_LEVEL_DEBUG, "PMU: AXP2202 DCDC1 = %dmv\n", pmu_axp2202_get_vol(i2c_dev, "dcdc1"));
    printk(LOG_LEVEL_DEBUG, "PMU: AXP2202 DCDC2 = %dmv\n", pmu_axp2202_get_vol(i2c_dev, "dcdc2"));
    printk(LOG_LEVEL_DEBUG, "PMU: AXP2202 DCDC3 = %dmv\n", pmu_axp2202_get_vol(i2c_dev, "dcdc3"));
    printk(LOG_LEVEL_DEBUG, "PMU: AXP2202 DCDC4 = %dmv\n", pmu_axp2202_get_vol(i2c_dev, "dcdc4"));
    printk(LOG_LEVEL_DEBUG, "PMU: AXP2202 ALDO1 = %dmv\n", pmu_axp2202_get_vol(i2c_dev, "aldo1"));
    printk(LOG_LEVEL_DEBUG, "PMU: AXP2202 ALDO2 = %dmv\n", pmu_axp2202_get_vol(i2c_dev, "aldo2"));
    printk(LOG_LEVEL_DEBUG, "PMU: AXP2202 ALDO3 = %dmv\n", pmu_axp2202_get_vol(i2c_dev, "aldo3"));
    printk(LOG_LEVEL_DEBUG, "PMU: AXP2202 ALDO4 = %dmv\n", pmu_axp2202_get_vol(i2c_dev, "aldo4"));
    printk(LOG_LEVEL_DEBUG, "PMU: AXP2202 BLDO1 = %dmv\n", pmu_axp2202_get_vol(i2c_dev, "bldo1"));
    printk(LOG_LEVEL_DEBUG, "PMU: AXP2202 BLDO2 = %dmv\n", pmu_axp2202_get_vol(i2c_dev, "bldo2"));
    printk(LOG_LEVEL_DEBUG, "PMU: AXP2202 BLDO3 = %dmv\n", pmu_axp2202_get_vol(i2c_dev, "bldo3"));
    printk(LOG_LEVEL_DEBUG, "PMU: AXP2202 BLDO4 = %dmv\n", pmu_axp2202_get_vol(i2c_dev, "bldo4"));
    printk(LOG_LEVEL_DEBUG, "PMU: AXP2202 CLDO1 = %dmv\n", pmu_axp2202_get_vol(i2c_dev, "cldo1"));
    printk(LOG_LEVEL_DEBUG, "PMU: AXP2202 CLDO2 = %dmv\n", pmu_axp2202_get_vol(i2c_dev, "cldo2"));
    printk(LOG_LEVEL_DEBUG, "PMU: AXP2202 CLDO3 = %dmv\n", pmu_axp2202_get_vol(i2c_dev, "cldo3"));
    printk(LOG_LEVEL_DEBUG, "PMU: AXP2202 CLDO4 = %dmv\n", pmu_axp2202_get_vol(i2c_dev, "cldo4"));
    printk(LOG_LEVEL_DEBUG, "PMU: AXP2202 CPUSLDO = %dmv\n", pmu_axp2202_get_vol(i2c_dev, "cpusldo"));
}