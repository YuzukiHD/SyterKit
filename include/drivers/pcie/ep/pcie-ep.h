/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file pcie-ep.h
 * @brief PCIe Endpoint (EP) layer.
 *
 * Provides the endpoint configuration header and BAR descriptions together
 * with the init/start/stop operations, BAR and ATU programming and MSI
 * configuration for the PCIe controller operating as an endpoint.
 */

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

/**
 * @struct pcie_ep_header
 * @brief PCIe Endpoint configuration-space header.
 */
struct pcie_ep_header {
	uint16_t vendor_id; /**< PCI vendor ID. */
	uint16_t device_id; /**< PCI device ID. */
	uint8_t revision_id; /**< Device revision ID. */
	uint8_t prog_if; /**< Programming interface class code byte. */
	uint8_t subclass; /**< Subclass class code byte. */
	uint8_t base_class; /**< Base class code byte. */
	uint8_t cache_line_size; /**< Cache line size. */
	uint16_t subsystem_vendor_id; /**< Subsystem vendor ID. */
	uint16_t subsystem_id; /**< Subsystem ID. */
	uint8_t interrupt_pin; /**< Interrupt pin. */
};

/**
 * @struct pcie_ep_bar
 * @brief Description of one Endpoint base address register (BAR).
 */
struct pcie_ep_bar {
	uint8_t bar; /**< BAR index. */
	uint64_t phys_addr; /**< Physical address backing the BAR. */
	uint64_t size; /**< BAR size, in bytes. */
	uint32_t flags; /**< BAR flags (IO, 64-bit, prefetchable). */
};

/**
 * @brief Initialize the PCIe controller as an Endpoint.
 */
int pcie_ep_init(struct pcie *pcie, const struct pcie_config *config);

/**
 * @brief Initialize the PCIe controller as an Endpoint from a device-tree node.
 */
int pcie_ep_init_dt(struct pcie *pcie, int node);

/**
 * @brief Write the Endpoint configuration-space header.
 */
int pcie_ep_write_header(struct pcie *pcie, uint8_t function,
		const struct pcie_ep_header *header);

/**
 * @brief Set up an Endpoint BAR.
 */
int pcie_ep_set_bar(struct pcie *pcie, uint8_t function,
		const struct pcie_ep_bar *bar);

/**
 * @brief Clear an Endpoint BAR.
 */
int pcie_ep_clear_bar(struct pcie *pcie, uint8_t function, uint8_t bar);

/**
 * @brief Program an inbound ATU translation window.
 */
int pcie_ep_program_inbound(struct pcie *pcie, uint8_t function,
		uint8_t index, enum pcie_atu_type type, uint64_t local_addr,
		uint64_t pci_addr, uint64_t size);

/**
 * @brief Program an outbound ATU translation window.
 */
int pcie_ep_program_outbound(struct pcie *pcie, uint8_t index,
		enum pcie_atu_type type, uint64_t local_addr, uint64_t pci_addr,
		uint64_t size);

/**
 * @brief Configure MSI for an Endpoint function.
 */
int pcie_ep_configure_msi(struct pcie *pcie, uint8_t function,
		uint8_t multiple_message_capable);

/**
 * @brief Start the Endpoint link.
 */
int pcie_ep_start(struct pcie *pcie);

/**
 * @brief Stop the Endpoint link.
 */
int pcie_ep_stop(struct pcie *pcie);

/**
 * @brief Check whether the Endpoint link is up.
 */
bool pcie_ep_link_up(struct pcie *pcie);

#endif /* __DRIVERS_PCIE_EP_H__ */
