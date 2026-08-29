/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file pcie.h
 * @brief High-level PCIe controller interface.
 *
 * Ties together the PCIe controller, PHY and reset GPIO into a single
 * pcie object and provides platform configuration and link management
 * entry points shared by the RC and EP layers.
 */

#ifndef __DRIVERS_PCIE_H__
#define __DRIVERS_PCIE_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <drivers/gpio/gpio.h>
#include <drivers/pcie/controller/pcie-controller.h>
#include <drivers/pcie/phy/pcie-phy.h>

/**
 * @struct pcie_config
 * @brief Platform configuration used to initialize a PCIe controller.
 */
struct pcie_config {
	enum pcie_mode mode; /**< Desired operating mode, RC or EP. */
	struct pcie_controller_config controller; /**< Controller hardware configuration. */
	/* NULL selects the built-in controller implementation. */
	const struct pcie_controller_ops *controller_ops; /**< Controller operations override. */
	struct pcie_phy_config phy; /**< PHY hardware configuration. */
	gpio_mux_t reset_gpio; /**< Reset GPIO pin description. */
	bool has_reset_gpio; /**< Whether a reset GPIO is provided. */
};

/**
 * @struct pcie
 * @brief Runtime PCIe controller instance.
 */
struct pcie {
	struct pcie_controller controller; /**< Controller sub-object. */
	struct pcie_phy phy; /**< PHY sub-object. */
	gpio_mux_t reset_gpio; /**< Reset GPIO pin description. */
	bool has_reset_gpio; /**< Whether a reset GPIO is provided. */
	enum pcie_mode mode; /**< Configured operating mode, RC or EP. */
	bool initialized; /**< Whether the instance has been initialized. */
};

/**
 * @brief Fill in the default sun55iw6 platform PCIe configuration.
 */
void pcie_config_sun55iw6(struct pcie_config *config, enum pcie_mode mode);

/**
 * @brief Power on the PCIe PHY and deassert the reset GPIO.
 */
int pcie_platform_power_on(const struct pcie_config *config);

/**
 * @brief Initialize the PCIe controller and PHY from a platform configuration.
 */
int pcie_init(struct pcie *pcie, const struct pcie_config *config);

/**
 * @brief Initialize the PCIe controller and PHY from a device-tree node.
 */
int pcie_init_dt(struct pcie *pcie, int node);

/**
 * @brief Shut down and release a PCIe instance.
 */
void pcie_exit(struct pcie *pcie);

/**
 * @brief Wait for the PCIe link to come up.
 */
int pcie_wait_for_link(struct pcie *pcie, uint32_t timeout_us);

/**
 * @brief Return the controller sub-object of a PCIe instance.
 *
 * @param[in] pcie PCIe instance.
 *
 * @return Pointer to the controller sub-object, or NULL if @p pcie is NULL.
 */
static inline struct pcie_controller *pcie_controller(struct pcie *pcie)
{
	return pcie != NULL ? &pcie->controller : NULL;
}

#endif /* __DRIVERS_PCIE_H__ */
