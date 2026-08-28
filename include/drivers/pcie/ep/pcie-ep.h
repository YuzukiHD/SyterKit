/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_PCIE_EP_H__
#define __DRIVERS_PCIE_EP_H__

#include <stdbool.h>
#include <stdint.h>

#include <drivers/pcie/pcie.h>

#define PCIE_EP_BAR_IO                  (1U << 0)
#define PCIE_EP_BAR_64BIT               (1U << 2)
#define PCIE_EP_BAR_PREFETCHABLE        (1U << 3)
#define PCIE_EP_BAR_FLAGS_MASK          (PCIE_EP_BAR_IO | \
					PCIE_EP_BAR_64BIT | PCIE_EP_BAR_PREFETCHABLE)

struct pcie_ep_header {
	uint16_t vendor_id;
	uint16_t device_id;
	uint8_t revision_id;
	uint8_t prog_if;
	uint8_t subclass;
	uint8_t base_class;
	uint8_t cache_line_size;
	uint16_t subsystem_vendor_id;
	uint16_t subsystem_id;
	uint8_t interrupt_pin;
};

struct pcie_ep_bar {
	uint8_t bar;
	uint64_t phys_addr;
	uint64_t size;
	uint32_t flags;
};

int pcie_ep_init(struct pcie *pcie, const struct pcie_config *config);
int pcie_ep_init_dt(struct pcie *pcie, int node);
int pcie_ep_write_header(struct pcie *pcie, uint8_t function,
		const struct pcie_ep_header *header);
int pcie_ep_set_bar(struct pcie *pcie, uint8_t function,
		const struct pcie_ep_bar *bar);
int pcie_ep_clear_bar(struct pcie *pcie, uint8_t function, uint8_t bar);
int pcie_ep_program_inbound(struct pcie *pcie, uint8_t function,
		uint8_t index, enum pcie_atu_type type, uint64_t local_addr,
		uint64_t pci_addr, uint64_t size);
int pcie_ep_program_outbound(struct pcie *pcie, uint8_t index,
		enum pcie_atu_type type, uint64_t local_addr, uint64_t pci_addr,
		uint64_t size);
int pcie_ep_configure_msi(struct pcie *pcie, uint8_t function,
		uint8_t multiple_message_capable);
int pcie_ep_start(struct pcie *pcie);
int pcie_ep_stop(struct pcie *pcie);
bool pcie_ep_link_up(struct pcie *pcie);

#endif /* __DRIVERS_PCIE_EP_H__ */
