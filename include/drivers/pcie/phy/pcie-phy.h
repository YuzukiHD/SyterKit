/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file pcie-phy.h
 * @brief PCIe PHY abstraction.
 *
 * Describes the PCIe PHY hardware configuration and the operations used to
 * initialize and shut down the PHY.
 */

#ifndef __DRIVERS_PCIE_PHY_H__
#define __DRIVERS_PCIE_PHY_H__

#include <stdbool.h>
#include <stdint.h>

#include <drivers/pcie/controller/pcie-controller.h>

struct pcie_phy;

/**
 * @struct pcie_phy_config
 * @brief Hardware configuration for the PCIe PHY.
 */
struct pcie_phy_config {
	uintptr_t subsys_base; /**< Base address of the PCIe subsystem. */
	uintptr_t phy_base; /**< Base address of the PHY registers. */
	uintptr_t ccu_base; /**< Base address of the clock control unit. */
	uint32_t timeout_us; /**< Timeout for PHY operations, in microseconds. */
};

/**
 * @struct pcie_phy_ops
 * @brief Operations implemented by the SoC-specific PCIe PHY driver.
 */
struct pcie_phy_ops {
	int (*init)(struct pcie_phy *phy); /**< Initialize the PHY. */
	void (*exit)(struct pcie_phy *phy); /**< Shut down the PHY. */
};

/**
 * @struct pcie_phy
 * @brief Runtime PCIe PHY instance.
 */
struct pcie_phy {
	struct pcie_phy_config config; /**< PHY hardware configuration. */
	const struct pcie_phy_ops *ops; /**< SoC-specific PHY operations. */
	bool initialized; /**< Whether the PHY has been initialized. */
};

extern const struct pcie_phy_ops pcie_phy_sun55iw6_ops;

/**
 * @brief Initialize the PCIe PHY.
 *
 * @param[out] phy PHY instance to initialize.
 * @param[in] config PHY hardware configuration.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int pcie_phy_init(struct pcie_phy *phy, const struct pcie_phy_config *config);

/**
 * @brief Shut down the PCIe PHY.
 *
 * @param[in] phy PHY instance to exit.
 */
void pcie_phy_exit(struct pcie_phy *phy);

#endif /* __DRIVERS_PCIE_PHY_H__ */
