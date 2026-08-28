/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stdint.h>

#include <log.h>
#include <timer.h>

#include <drivers/pcie/rc/pcie-rc.h>
#include <dt-compatible/pcie-dt.h>

void pcie_rc_config_default(struct pcie_rc_config *config)
{
	if (config == NULL)
		return;
	*config = (struct pcie_rc_config){
		.primary_bus = 0U,
		.secondary_bus = 1U,
		.subordinate_bus = 0xffU,
		.enable_io = true,
		.enable_memory = true,
		.enable_master = true,
	};
}

int pcie_rc_setup(struct pcie *pcie, const struct pcie_rc_config *config)
{
	struct pcie_rc_config defaults;
	uint32_t value;
	struct pcie_atu_region region;
	int ret;

	if (pcie == NULL || !pcie->initialized || pcie->mode != PCIE_MODE_RC)
		return PCIE_ERR_INVALID;
	if (config == NULL) {
		pcie_rc_config_default(&defaults);
		config = &defaults;
	}
	if (config->secondary_bus <= config->primary_bus ||
	    config->subordinate_bus < config->secondary_bus)
		return PCIE_ERR_INVALID;

	ret = pcie_controller_dbi_write(&pcie->controller, PCIE_CFG_BAR0, 4U, 0x4U);
	if (ret)
		return ret;
	ret = pcie_controller_dbi_write(&pcie->controller, PCIE_CFG_BAR0 + 4U, 4U, 0U);
	if (ret)
		return ret;
	ret = pcie_controller_dbi_read(&pcie->controller, PCIE_CFG_INTERRUPT_LINE, 4U,
		&value);
	if (ret)
		return ret;
	value = (value & 0xffff00ffU) | 0x00000100U;
	ret = pcie_controller_dbi_write(&pcie->controller, PCIE_CFG_INTERRUPT_LINE, 4U,
		value);
	if (ret)
		return ret;
	ret = pcie_controller_dbi_read(&pcie->controller, PCIE_CFG_PRIMARY_BUS, 4U,
		&value);
	if (ret)
		return ret;
	value = (value & 0xff000000U) |
		(uint32_t)config->primary_bus |
		((uint32_t)config->secondary_bus << 8) |
		((uint32_t)config->subordinate_bus << 16);
	ret = pcie_controller_dbi_write(&pcie->controller, PCIE_CFG_PRIMARY_BUS, 4U,
		value);
	if (ret)
		return ret;
	ret = pcie_controller_dbi_read(&pcie->controller, PCIE_CFG_COMMAND, 4U, &value);
	if (ret)
		return ret;
	value &= 0xffff0000U;
	if (config->enable_io)
		value |= PCIE_CFG_COMMAND_IO;
	if (config->enable_memory)
		value |= PCIE_CFG_COMMAND_MEMORY;
	if (config->enable_master)
		value |= PCIE_CFG_COMMAND_MASTER;
	value |= PCIE_CFG_COMMAND_SERR;
	ret = pcie_controller_dbi_write(&pcie->controller, PCIE_CFG_COMMAND, 4U, value);
	if (ret)
		return ret;

	if (config->enable_memory) {
		region = (struct pcie_atu_region){
			.direction = PCIE_ATU_OUTBOUND,
			.type = PCIE_ATU_TYPE_MEM,
			.index = 1U,
			.cpu_addr = pcie->controller.config.mem_cpu_addr,
			.pci_addr = pcie->controller.config.mem_pci_addr,
			.size = pcie->controller.config.mem_size,
		};
		ret = pcie_controller_program_atu(&pcie->controller, &region);
		if (ret)
			return ret;
	}
	if (config->enable_io) {
		region = (struct pcie_atu_region){
			.direction = PCIE_ATU_OUTBOUND,
			.type = PCIE_ATU_TYPE_IO,
			.index = 2U,
			.cpu_addr = pcie->controller.config.io_cpu_addr,
			.pci_addr = pcie->controller.config.io_pci_addr,
			.size = pcie->controller.config.io_size,
		};
		ret = pcie_controller_program_atu(&pcie->controller, &region);
		if (ret)
			return ret;
	}
	/* BAR0 is only a temporary root-port setup BAR. */
	ret = pcie_controller_dbi_write(&pcie->controller, PCIE_CFG_BAR0, 4U, 0U);
	if (ret)
		return ret;

	ret = pcie_controller_dbi_ro_write_enable(&pcie->controller, true);
	if (ret)
		return ret;
	ret = pcie_controller_dbi_write(&pcie->controller, PCIE_CFG_CLASS_CODE, 2U, 0x0604U);
	if (pcie_controller_dbi_ro_write_enable(&pcie->controller, false) != PCIE_OK)
		return PCIE_ERR_IO;
	if (ret)
		return ret;
	return PCIE_OK;
}

static int pcie_rc_reset_endpoint(struct pcie *pcie)
{
	if (!pcie->has_reset_gpio)
		return PCIE_OK;
#if defined(CONFIG_DRIVER_GPIO_V1) || defined(CONFIG_DRIVER_GPIO_V2) || \
	defined(CONFIG_DRIVER_GPIO_V3) || defined(CONFIG_DRIVER_GPIO_V4)
	sunxi_gpio_init(&pcie->reset_gpio);
	sunxi_gpio_set_value(&pcie->reset_gpio, GPIO_LEVEL_LOW);
	udelay(100000U);
	sunxi_gpio_set_value(&pcie->reset_gpio, GPIO_LEVEL_HIGH);
	return PCIE_OK;
#else
	return PCIE_ERR_UNSUPPORTED;
#endif
}

int pcie_rc_init(struct pcie *pcie, const struct pcie_config *config,
		const struct pcie_rc_config *rc_config)
{
	int ret;

	ret = pcie_init(pcie, config);
	if (ret)
		return ret;
	ret = pcie_rc_setup(pcie, rc_config);
	if (ret)
		pcie_exit(pcie);
	return ret;
}

int pcie_rc_init_dt(struct pcie *pcie, int node,
		const struct pcie_rc_config *rc_config)
{
	struct pcie_config config;

	if (sunxi_pcie_dt_read_config(&config, node) != DRIVER_OK ||
	    config.mode != PCIE_MODE_RC)
		return PCIE_ERR_INVALID;
	return pcie_rc_init(pcie, &config, rc_config);
}

int pcie_rc_start(struct pcie *pcie, uint32_t timeout_us)
{
	int ret;

	if (pcie == NULL || !pcie->initialized || pcie->mode != PCIE_MODE_RC)
		return PCIE_ERR_INVALID;
	ret = pcie_controller_ltssm(&pcie->controller, false);
	if (ret)
		return ret;
	ret = pcie_rc_reset_endpoint(pcie);
	if (ret)
		return ret;
	ret = pcie_controller_ltssm(&pcie->controller, true);
	if (ret)
		return ret;
	ret = pcie_controller_wait_link(&pcie->controller, timeout_us);
	if (ret) {
		pcie_controller_ltssm(&pcie->controller, false);
		printk_error("PCIe: link training timeout\n");
		return ret;
	}
	if (pcie->controller.config.link_gen > 1U) {
		ret = pcie_controller_change_speed(&pcie->controller,
			pcie->controller.config.link_gen);
		if (ret) {
			pcie_controller_ltssm(&pcie->controller, false);
			return ret;
		}
	}
	return PCIE_OK;
}

int pcie_rc_stop(struct pcie *pcie)
{
	if (pcie == NULL || !pcie->initialized || pcie->mode != PCIE_MODE_RC)
		return PCIE_ERR_INVALID;
	return pcie_controller_ltssm(&pcie->controller, false);
}

bool pcie_rc_link_up(struct pcie *pcie)
{
	if (pcie == NULL || !pcie->initialized || pcie->mode != PCIE_MODE_RC)
		return false;
	return pcie_controller_link_up(&pcie->controller);
}

int pcie_rc_read_config(struct pcie *pcie, uint32_t bdf, uint32_t offset,
		uint8_t size, uint32_t *value)
{
	if (pcie == NULL || !pcie->initialized || pcie->mode != PCIE_MODE_RC)
		return PCIE_ERR_INVALID;
	return pcie_controller_cfg_read(&pcie->controller, bdf, offset, size, value);
}

int pcie_rc_write_config(struct pcie *pcie, uint32_t bdf, uint32_t offset,
		uint8_t size, uint32_t value)
{
	if (pcie == NULL || !pcie->initialized || pcie->mode != PCIE_MODE_RC)
		return PCIE_ERR_INVALID;
	return pcie_controller_cfg_write(&pcie->controller, bdf, offset, size, value);
}

int pcie_rc_program_outbound(struct pcie *pcie, uint8_t index,
		enum pcie_atu_type type, uint64_t cpu_addr, uint64_t pci_addr,
		uint64_t size)
{
	struct pcie_atu_region region;

	if (pcie == NULL || !pcie->initialized || pcie->mode != PCIE_MODE_RC)
		return PCIE_ERR_INVALID;
	region = (struct pcie_atu_region){
		.direction = PCIE_ATU_OUTBOUND,
		.type = type,
		.index = index,
		.cpu_addr = cpu_addr,
		.pci_addr = pci_addr,
		.size = size,
	};
	return pcie_controller_program_atu(&pcie->controller, &region);
}
