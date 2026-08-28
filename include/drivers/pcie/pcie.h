/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_PCIE_H__
#define __DRIVERS_PCIE_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <drivers/gpio/gpio.h>
#include <drivers/pcie/controller/pcie-controller.h>
#include <drivers/pcie/phy/pcie-phy.h>

struct pcie_config {
	enum pcie_mode mode;
	struct pcie_controller_config controller;
	/* NULL selects the built-in controller implementation. */
	const struct pcie_controller_ops *controller_ops;
	struct pcie_phy_config phy;
	gpio_mux_t reset_gpio;
	bool has_reset_gpio;
};

struct pcie {
	struct pcie_controller controller;
	struct pcie_phy phy;
	gpio_mux_t reset_gpio;
	bool has_reset_gpio;
	enum pcie_mode mode;
	bool initialized;
};

void pcie_config_sun55iw6(struct pcie_config *config, enum pcie_mode mode);
int pcie_platform_power_on(const struct pcie_config *config);
int pcie_init(struct pcie *pcie, const struct pcie_config *config);
int pcie_init_dt(struct pcie *pcie, int node);
void pcie_exit(struct pcie *pcie);
int pcie_wait_for_link(struct pcie *pcie, uint32_t timeout_us);

static inline struct pcie_controller *pcie_controller(struct pcie *pcie)
{
	return pcie != NULL ? &pcie->controller : NULL;
}

#endif /* __DRIVERS_PCIE_H__ */
