/* SPDX-License-Identifier:	GPL-2.0+ */

#ifndef __DRIVERS_USB_CONTROLLER_H__
#define __DRIVERS_USB_CONTROLLER_H__

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <drivers/usb/usb_types.h>
#include <drivers/usb/usb_regs.h>

#define USBC_MAX_OPEN_NUM 8
#define USBC_MAX_CTL_NUM 3
#define USBC_MAX_EP_NUM 6
#define USBC0_MAX_FIFO_SIZE (8 * 1024)
#define USBC_EP0_FIFOSIZE 64

/** @brief FIFO address and size assigned to a USB port. */
typedef struct fifo_info {
	uint32_t port0_fifo_addr;
	uint32_t port0_fifo_size;
} fifo_info_t;

/** @brief Hardware state associated with an opened USB controller port. */
typedef struct usb_controller_otg {
	uint32_t port_num;	/* USB port number */
	uint32_t base_addr; /* USB base address */

	uint32_t used; /* Whether it is currently being used */
	uint32_t no;   /* Position in the management array */
} usb_controller_otg_t;

/**
 * @brief Get the interrupt pending flag of a TX endpoint.
 *
 * @param addr The address of the USB controller.
 * @return Returns the interrupt pending flag of the TX endpoint.
 */
static inline uint32_t usb_controller_int_tx_pending(uint32_t addr) {
	return readw(USBC_REG_INTTx(addr));
}

/**
 * @brief Clear the interrupt pending flag of a TX endpoint.
 *
 * @param addr The address of the USB controller.
 * @param ep_index The index of the TX endpoint.
 */
static inline void usb_controller_int_clear_tx_pending(uint32_t addr, uint8_t ep_index) {
	writew((1 << ep_index), USBC_REG_INTTx(addr));
}

/**
 * @brief Clear the interrupt pending flags of all TX endpoints.
 *
 * @param addr The address of the USB controller.
 */
static inline void usb_controller_int_clear_tx_pending_all(uint32_t addr) {
	writew(0xffff, USBC_REG_INTTx(addr));
}

/**
 * @brief Get the interrupt pending flag of an RX endpoint.
 *
 * @param addr The address of the USB controller.
 * @return Returns the interrupt pending flag of the RX endpoint.
 */
static inline uint32_t usb_controller_int_rx_pending(uint32_t addr) {
	return readw(USBC_REG_INTRx(addr));
}

/**
 * @brief Clear the interrupt pending flag of an RX endpoint.
 *
 * @param addr The address of the USB controller.
 * @param ep_index The index of the RX endpoint.
 */
static inline void usb_controller_int_clear_rx_pending(uint32_t addr, uint8_t ep_index) {
	writew((1 << ep_index), USBC_REG_INTRx(addr));
}

/**
 * @brief Clear the interrupt pending flags of all RX endpoints.
 *
 * @param addr The address of the USB controller.
 */
static inline void usb_controller_int_clear_rx_pending_all(uint32_t addr) {
	writew(0xffff, USBC_REG_INTRx(addr));
}

/**
 * @brief Enable the interrupt of a TX endpoint.
 *
 * @param addr The address of the USB controller.
 * @param ep_index The index of the TX endpoint.
 */
static inline void usb_controller_int_enable_tx_ep(uint32_t addr, uint8_t ep_index) {
	usb_set_bit16(ep_index, USBC_REG_INTTxE(addr));
}

/**
 * @brief Enable the interrupt of an RX endpoint.
 *
 * @param addr The address of the USB controller.
 * @param ep_index The index of the RX endpoint.
 */
static inline void usb_controller_int_enable_rx_ep(uint32_t addr, uint8_t ep_index) {
	usb_set_bit16(ep_index, USBC_REG_INTRxE(addr));
}

/**
 * @brief Disable the interrupt of a TX endpoint.
 *
 * @param addr The address of the USB controller.
 * @param ep_index The index of the TX endpoint.
 */
static inline void usb_controller_int_disable_tx_ep(uint32_t addr, uint8_t ep_index) {
	usb_clear_bit16(ep_index, USBC_REG_INTTxE(addr));
}

/**
 * @brief Disable the interrupt of an RX endpoint.
 *
 * @param addr The address of the USB controller.
 * @param ep_index The index of the RX endpoint.
 */
static inline void usb_controller_int_disable_rx_ep(uint32_t addr, uint8_t ep_index) {
	usb_clear_bit16(ep_index, USBC_REG_INTRxE(addr));
}

/**
 * @brief Disable the interrupts of all TX endpoints.
 *
 * @param addr The address of the USB controller.
 */
static inline void usb_controller_int_disable_tx_all(uint32_t addr) {
	writew(0, USBC_REG_INTTxE(addr));
}

/**
 * @brief Disable the interrupts of all RX endpoints.
 *
 * @param addr The address of the USB controller.
 */
static inline void usb_controller_int_disable_rx_all(uint32_t addr) {
	writew(0, USBC_REG_INTRxE(addr));
}

/**
 * @brief Open the USB OTG controller.
 *
 * @param otg_no The OTG controller number.
 * @return Returns the status of the operation.
 */
uintptr_t usb_controller_open_otg(uint32_t otg_no);

/**
 * @brief Set the memory-mapped controller base selected from the devicetree.
 *
 * @param otg_no The OTG controller number.
 * @param base The controller register base address.
 * @return Zero on success, or a negative error code on invalid input.
 */
int usb_controller_set_base(uint32_t otg_no, uint32_t base);

/**
 * @brief Close the USB OTG controller.
 *
 * @param husb The handle to the USB controller.
 * @return Returns the status of the operation.
 */
int usb_controller_close_otg(uintptr_t husb);

/**
 * @brief Set the ID status for the USB controller.
 *
 * @param husb The handle to the USB controller.
 * @param id_type The type of ID status to set.
 */
void usb_controller_force_id_status(uintptr_t husb, uint32_t id_type);

/**
 * @brief Force the VBUS valid state for the USB controller.
 *
 * @param husb The handle to the USB controller.
 * @param vbus_type The type of VBUS state to force.
 */
void usb_controller_force_vbus_valid(uintptr_t husb, uint32_t vbus_type);

/**
 * @brief Enable the ID pull-up resistor for the USB controller.
 *
 * @param husb The handle to the USB controller.
 */
void usb_controller_id_pull_enable(uintptr_t husb);

/**
 * @brief Disable the ID pull-up resistor for the USB controller.
 *
 * @param husb The handle to the USB controller.
 */
void usb_controller_id_pull_disable(uintptr_t husb);

/**
 * @brief Enable the DP/DM pull-up resistors for the USB controller.
 *
 * @param husb The handle to the USB controller.
 */
void usb_controller_dpdm_pull_enable(uintptr_t husb);

/**
 * @brief Disable the DP/DM pull-up resistors for the USB controller.
 *
 * @param husb The handle to the USB controller.
 */
void usb_controller_dpdm_pull_disable(uintptr_t husb);

/**
 * @brief Select the PIO or DMA path for the active endpoint.
 *
 * @param husb The handle to the USB controller.
 * @param io_type The transfer I/O mode.
 * @param ep_type The endpoint type.
 * @param ep_index The endpoint index.
 */
void usb_controller_select_bus(uintptr_t husb, uint32_t io_type, uint32_t ep_type, uint32_t ep_index);

/**
 * @brief Disable all miscellaneous USB interrupts.
 *
 * @param husb The handle to the USB controller.
 */
void usb_controller_int_disable_usb_misc_all(uintptr_t husb);

/**
 * @brief Disable all endpoint-specific interrupts.
 *
 * @param husb The handle to the USB controller.
 * @param ep_type The type of endpoint to disable interrupts for.
 */
void usb_controller_int_disable_ep_all(uintptr_t husb, uint32_t ep_type);

/**
 * @brief Enable specific miscellaneous USB interrupts.
 *
 * @param husb The handle to the USB controller.
 * @param mask The interrupt mask to enable.
 */
void usb_controller_int_enable_usb_misc_uint(uintptr_t husb, uint32_t mask);

/**
 * @brief Disable specific miscellaneous USB interrupts.
 *
 * @param husb The handle to the USB controller.
 * @param mask The interrupt mask to enable.
 */
void usb_controller_int_disable_usb_misc_uint(uintptr_t husb, uint32_t mask);

/**
 * @brief Enable interrupts for a specific endpoint.
 *
 * @param husb The handle to the USB controller.
 * @param ep_type The type of endpoint to enable interrupts for.
 * @param ep_index The index of the endpoint to enable interrupts for.
 */
void usb_controller_int_enable_ep(uintptr_t husb, uint32_t ep_type, uint32_t ep_index);

/**
 * @brief Get the pending interrupt status for a specific endpoint.
 *
 * @param husb The handle to the USB controller.
 * @param ep_type The type of endpoint.
 * @return Returns the pending interrupt status.
 */
uint32_t usb_controller_int_ep_pending(uintptr_t husb, uint32_t ep_type);

/**
 * @brief Clear the pending interrupt flag for a specific endpoint.
 *
 * @param husb The handle to the USB controller.
 * @param ep_type The type of endpoint.
 * @param ep_index The index of the endpoint.
 */
void usb_controller_int_clear_ep_pending(uintptr_t husb, uint32_t ep_type, uint8_t ep_index);

/**
 * @brief Clear the pending interrupt flags for all endpoints of a specific type.
 *
 * @param husb The handle to the USB controller.
 * @param ep_type The type of endpoint.
 */
void usb_controller_int_clear_ep_pending_all(uintptr_t husb, uint32_t ep_type);

/**
 * @brief Get the pending miscellaneous interrupt status.
 *
 * @param husb The handle to the USB controller.
 * @return Returns the pending miscellaneous interrupt status.
 */
uint32_t usb_controller_int_misc_pending(uintptr_t husb);

/**
 * @brief Clear the pending miscellaneous interrupt flag.
 *
 * @param husb The handle to the USB controller.
 * @param mask The interrupt mask to clear.
 */
void usb_controller_int_clear_misc_pending(uintptr_t husb, uint32_t mask);

/**
 * @brief Clear the pending miscellaneous interrupt flags for all interrupts.
 *
 * @param husb The handle to the USB controller.
 */
void usb_controller_int_clear_misc_pending_all(uintptr_t husb);

/**
 * @brief Get the active endpoint for the USB controller.
 *
 * @param husb The handle to the USB controller.
 * @return Returns the active endpoint.
 */
uint32_t usb_controller_get_active_ep(uintptr_t husb);

/**
 * @brief Disable interrupts for a specific endpoint.
 *
 * @param husb The handle to the USB controller.
 * @param ep_type The type of endpoint.
 * @param ep_index The index of the endpoint.
 */
void usb_controller_int_disable_ep(uintptr_t husb, uint32_t ep_type, uint8_t ep_index);

/**
 * @brief Select the active endpoint for the USB controller.
 *
 * @param husb The handle to the USB controller.
 * @param ep_index The index of the endpoint to select as active.
 */
void usb_controller_select_active_ep(uintptr_t husb, uint8_t ep_index);

/**
 * @brief Configure the FIFO for a default transmit endpoint.
 *
 * @param addr The address of the USB controller.
 */
void usb_controller_config_fifo_tx_ep_default(uint32_t addr);

/**
 * @brief Configure the FIFO for a transmit endpoint.
 *
 * @param addr The address of the USB controller.
 * @param is_double_fifo Whether the endpoint has double buffering enabled.
 * @param fifo_size The size of the FIFO.
 * @param fifo_addr The base address of the FIFO.
 */
void usb_controller_config_fifo_tx_ep(uint32_t addr, uint32_t is_double_fifo, uint32_t fifo_size, uint32_t fifo_addr);

/**
 * @brief Configure the FIFO for a default receive endpoint.
 *
 * @param addr The address of the USB controller.
 */
void usb_controller_config_fifo_rx_ep_default(uint32_t addr);

/**
 * @brief Configure the FIFO for a receive endpoint.
 *
 * @param addr The address of the USB controller.
 * @param is_double_fifo Whether the endpoint has double buffering enabled.
 * @param fifo_size The size of the FIFO.
 * @param fifo_addr The base address of the FIFO.
 */
void usb_controller_config_fifo_rx_ep(uint32_t addr, uint32_t is_double_fifo, uint32_t fifo_size, uint32_t fifo_addr);

/**
 * @brief Configure the FIFO for a specific endpoint.
 *
 * @param husb The handle to the USB controller.
 * @param ep_type The type of endpoint.
 * @param is_double_fifo Whether the endpoint has double buffering enabled.
 * @param fifo_size The size of the FIFO.
 * @param fifo_addr The base address of the FIFO.
 */
void usb_controller_config_fifo(uintptr_t husb, uint32_t ep_type, uint32_t is_double_fifo, uint32_t fifo_size, uint32_t fifo_addr);

/**
 * @brief Read the VBUS state reported by the USB controller.
 *
 * @param husb The handle to the USB controller.
 * @return The decoded VBUS state.
 */
uint32_t usb_controller_get_vbus_status(uintptr_t husb);

/**
 * @brief Read the number of bytes available in an endpoint FIFO.
 *
 * @param husb The handle to the USB controller.
 * @param ep_type The endpoint type.
 * @return The number of bytes reported by the endpoint FIFO.
 */
uint32_t usb_controller_read_len_from_fifo(uintptr_t husb, uint32_t ep_type);

/**
 * @brief Write a packet to a controller FIFO.
 *
 * @param husb The handle to the USB controller.
 * @param fifo The FIFO address.
 * @param cnt The number of bytes to write.
 * @param buff The source buffer.
 * @return The number of bytes written.
 */
uint32_t usb_controller_write_packet(uintptr_t husb, uint32_t fifo, uint32_t cnt, const void *buff);

/**
 * @brief Read a packet from a controller FIFO.
 *
 * @param husb The handle to the USB controller.
 * @param fifo The FIFO address.
 * @param cnt The number of bytes to read.
 * @param buff The destination buffer.
 * @return The number of bytes read.
 */
uint32_t usb_controller_read_packet(uintptr_t husb, uint32_t fifo, uint32_t cnt, void *buff);

/**
 * @brief Map the controller SRAM region to the USB FIFO.
 *
 * @param husb The handle to the USB controller.
 * @param sram_base The base address of the SRAM region.
 */
void usb_controller_config_fifo_base(uintptr_t husb, uint32_t sram_base);

/**
 * @brief Return the FIFO start address assigned to the controller port.
 *
 * @param husb The handle to the USB controller.
 * @return The FIFO start address.
 */
uint32_t usb_controller_get_port_fifo_start_addr(uintptr_t husb);

/**
 * @brief Return the FIFO size assigned to the controller port.
 *
 * @param husb The handle to the USB controller.
 * @return The FIFO size in bytes.
 */
uint32_t usb_controller_get_port_fifo_size(uintptr_t husb);

/**
 * @brief Return the FIFO address for an endpoint.
 *
 * @param husb The handle to the USB controller.
 * @param ep_index The endpoint index.
 * @return The endpoint FIFO address, or zero for an invalid controller.
 */
uint32_t usb_controller_select_fifo(uintptr_t husb, uint32_t ep_index);

#endif /* __DRIVERS_USB_CONTROLLER_H__ */
