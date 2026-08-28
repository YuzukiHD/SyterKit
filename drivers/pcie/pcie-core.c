/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <dt2c/driver.h>

#include <drivers/pcie/pcie.h>
#include <dt-compatible/pcie-dt.h>

#define SUN55IW6_PCIE_DBI_BASE          0x04800000UL
#define SUN55IW6_PCIE_APP_BASE          0x04c00000UL
#define SUN55IW6_PCIE_SUBSYS_BASE       0x04f00000UL
#define SUN55IW6_PCIE_PHY_BASE          0x04f80000UL
#define SUN55IW6_PCIE_CCU_BASE          0x02002000UL

#define SUN55IW6_PCIE_CFG_BASE          0x20000000ULL
#define SUN55IW6_PCIE_IO_BASE           0x21000000ULL
#define SUN55IW6_PCIE_MEM_BASE          0x22000000ULL
#define SUN55IW6_PCIE_EP_FUNCTION_STRIDE 0x10000U

int __attribute__((weak)) pcie_platform_power_on(
		const struct pcie_config *config)
{
	(void)config;
	return PCIE_OK;
}

void pcie_config_sun55iw6(struct pcie_config *config, enum pcie_mode mode)
{
	if (config == NULL)
		return;
	memset(config, 0, sizeof(*config));
	config->mode = mode;
	config->controller.dbi_base = SUN55IW6_PCIE_DBI_BASE;
	config->controller.app_base = SUN55IW6_PCIE_APP_BASE;
	config->controller.dbi_size = 0x480000U;
	config->controller.app_size = 0x1000U;
	config->controller.cfg_cpu_addr = (uintptr_t)SUN55IW6_PCIE_CFG_BASE;
	config->controller.cfg_pci_addr = SUN55IW6_PCIE_CFG_BASE;
	config->controller.cfg_size = 0x01000000U;
	config->controller.io_cpu_addr = (uintptr_t)SUN55IW6_PCIE_IO_BASE;
	config->controller.io_pci_addr = SUN55IW6_PCIE_IO_BASE;
	config->controller.io_size = 0x01000000U;
	config->controller.mem_cpu_addr = (uintptr_t)SUN55IW6_PCIE_MEM_BASE;
	config->controller.mem_pci_addr = SUN55IW6_PCIE_MEM_BASE;
	config->controller.mem_size = 0x06000000U;
	config->controller.mode = mode;
	config->controller.lanes = 1U;
	config->controller.max_lanes = 1U;
	config->controller.link_gen = 2U;
	config->controller.max_link_gen = 2U;
	config->controller.num_ob_windows = 8U;
	config->controller.num_ib_windows = 8U;
	config->controller.ep_function_stride = SUN55IW6_PCIE_EP_FUNCTION_STRIDE;
	config->controller.timeout_us = 1000000U;
	config->phy.subsys_base = SUN55IW6_PCIE_SUBSYS_BASE;
	config->phy.phy_base = SUN55IW6_PCIE_PHY_BASE;
	config->phy.ccu_base = SUN55IW6_PCIE_CCU_BASE;
	config->phy.timeout_us = 1000000U;
}

int pcie_init(struct pcie *pcie, const struct pcie_config *config)
{
	int ret;

	if (pcie == NULL || config == NULL ||
	    (config->mode != PCIE_MODE_RC && config->mode != PCIE_MODE_EP))
		return PCIE_ERR_INVALID;
	ret = pcie_platform_power_on(config);
	if (ret)
		return ret;
	*pcie = (struct pcie){ 0 };
	pcie->mode = config->mode;
	pcie->reset_gpio = config->reset_gpio;
	pcie->has_reset_gpio = config->has_reset_gpio;
	ret = pcie_phy_init(&pcie->phy, &config->phy);
	if (ret)
		return ret;
	if (config->controller_ops != NULL)
		ret = pcie_controller_init_with_ops(&pcie->controller,
			&config->controller, config->controller_ops);
	else
		ret = pcie_controller_init(&pcie->controller, &config->controller);
	if (ret) {
		pcie_phy_exit(&pcie->phy);
		return ret;
	}
	ret = pcie_controller_ltssm(&pcie->controller, false);
	if (ret) {
		pcie_controller_exit(&pcie->controller);
		pcie_phy_exit(&pcie->phy);
		return ret;
	}
	ret = pcie_controller_set_mode(&pcie->controller, config->mode);
	if (!ret)
		ret = pcie_controller_set_link(&pcie->controller);
	if (ret) {
		pcie_controller_exit(&pcie->controller);
		pcie_phy_exit(&pcie->phy);
		return ret;
	}
	pcie->initialized = true;
	return PCIE_OK;
}

int pcie_init_dt(struct pcie *pcie, int node)
{
	struct pcie_config config;

	if (sunxi_pcie_dt_read_config(&config, node) != DRIVER_OK)
		return PCIE_ERR_INVALID;
	return pcie_init(pcie, &config);
}

void pcie_exit(struct pcie *pcie)
{
	if (pcie == NULL || !pcie->initialized)
		return;
	pcie_controller_ltssm(&pcie->controller, false);
	pcie_controller_exit(&pcie->controller);
	pcie_phy_exit(&pcie->phy);
	pcie->initialized = false;
}

int pcie_wait_for_link(struct pcie *pcie, uint32_t timeout_us)
{
	if (pcie == NULL || !pcie->initialized)
		return PCIE_ERR_INVALID;
	return pcie_controller_wait_link(&pcie->controller, timeout_us);
}

DT2C_DRIVER_COMPAT("allwinner,sun55iw6-pcie-rc");
DT2C_DRIVER_COMPAT("allwinner,sun55iw6-pcie-ep");
