/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_PCIE_RC_H__
#define __DRIVERS_PCIE_RC_H__

#include <stdbool.h>
#include <stdint.h>

#include <drivers/pcie/pcie.h>

struct pcie_rc_config {
	uint8_t primary_bus;
	uint8_t secondary_bus;
	uint8_t subordinate_bus;
	bool enable_io;
	bool enable_memory;
	bool enable_master;
};

void pcie_rc_config_default(struct pcie_rc_config *config);
int pcie_rc_init(struct pcie *pcie, const struct pcie_config *config,
		const struct pcie_rc_config *rc_config);
int pcie_rc_init_dt(struct pcie *pcie, int node,
		const struct pcie_rc_config *rc_config);
int pcie_rc_setup(struct pcie *pcie, const struct pcie_rc_config *config);
int pcie_rc_start(struct pcie *pcie, uint32_t timeout_us);
int pcie_rc_stop(struct pcie *pcie);
bool pcie_rc_link_up(struct pcie *pcie);
int pcie_rc_read_config(struct pcie *pcie, uint32_t bdf, uint32_t offset,
		uint8_t size, uint32_t *value);
int pcie_rc_write_config(struct pcie *pcie, uint32_t bdf, uint32_t offset,
		uint8_t size, uint32_t value);
int pcie_rc_program_outbound(struct pcie *pcie, uint8_t index,
		enum pcie_atu_type type, uint64_t cpu_addr, uint64_t pci_addr,
		uint64_t size);

#endif /* __DRIVERS_PCIE_RC_H__ */
