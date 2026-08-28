/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_PCIE_PHY_H__
#define __DRIVERS_PCIE_PHY_H__

#include <stdbool.h>
#include <stdint.h>

#include <drivers/pcie/controller/pcie-controller.h>

struct pcie_phy;

struct pcie_phy_config {
	uintptr_t subsys_base;
	uintptr_t phy_base;
	uintptr_t ccu_base;
	uint32_t timeout_us;
};

struct pcie_phy_ops {
	int (*init)(struct pcie_phy *phy);
	void (*exit)(struct pcie_phy *phy);
};

struct pcie_phy {
	struct pcie_phy_config config;
	const struct pcie_phy_ops *ops;
	bool initialized;
};

extern const struct pcie_phy_ops pcie_phy_sun55iw6_ops;

int pcie_phy_init(struct pcie_phy *phy, const struct pcie_phy_config *config);
void pcie_phy_exit(struct pcie_phy *phy);

#endif /* __DRIVERS_PCIE_PHY_H__ */
