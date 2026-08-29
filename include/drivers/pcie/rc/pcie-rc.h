/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file pcie-rc.h
 * @brief PCIe Root Complex (RC) layer.
 *
 * Provides the RC configuration and the init/setup/start/stop operations,
 * configuration-space access and outbound ATU programming for the PCIe
 * controller operating as a root complex.
 */

#ifndef __DRIVERS_PCIE_RC_H__
#define __DRIVERS_PCIE_RC_H__

#include <stdbool.h>
#include <stdint.h>

#include <drivers/pcie/pcie.h>

/**
 * @struct pcie_rc_config
 * @brief Root Complex bus configuration.
 */
struct pcie_rc_config {
	uint8_t primary_bus; /**< Primary bus number. */
	uint8_t secondary_bus; /**< Secondary bus number. */
	uint8_t subordinate_bus; /**< Highest subordinate bus number. */
	bool enable_io; /**< Enable IO space in the command register. */
	bool enable_memory; /**< Enable memory space in the command register. */
	bool enable_master; /**< Enable bus mastering in the command register. */
};

/**
 * @brief Fill in the default Root Complex configuration.
 */
void pcie_rc_config_default(struct pcie_rc_config *config);

/**
 * @brief Initialize the PCIe controller as a Root Complex.
 */
int pcie_rc_init(struct pcie *pcie, const struct pcie_config *config,
		const struct pcie_rc_config *rc_config);

/**
 * @brief Initialize the PCIe controller as a Root Complex from a device-tree node.
 */
int pcie_rc_init_dt(struct pcie *pcie, int node,
		const struct pcie_rc_config *rc_config);

/**
 * @brief Set up the Root Complex bus window and command registers.
 */
int pcie_rc_setup(struct pcie *pcie, const struct pcie_rc_config *config);

/**
 * @brief Start the Root Complex link.
 */
int pcie_rc_start(struct pcie *pcie, uint32_t timeout_us);

/**
 * @brief Stop the Root Complex link.
 */
int pcie_rc_stop(struct pcie *pcie);

/**
 * @brief Check whether the Root Complex link is up.
 */
bool pcie_rc_link_up(struct pcie *pcie);

/**
 * @brief Read a value from Root Complex configuration space.
 */
int pcie_rc_read_config(struct pcie *pcie, uint32_t bdf, uint32_t offset,
		uint8_t size, uint32_t *value);

/**
 * @brief Write a value to Root Complex configuration space.
 */
int pcie_rc_write_config(struct pcie *pcie, uint32_t bdf, uint32_t offset,
		uint8_t size, uint32_t value);

/**
 * @brief Program an outbound ATU translation window.
 */
int pcie_rc_program_outbound(struct pcie *pcie, uint8_t index,
		enum pcie_atu_type type, uint64_t cpu_addr, uint64_t pci_addr,
		uint64_t size);

#endif /* __DRIVERS_PCIE_RC_H__ */
