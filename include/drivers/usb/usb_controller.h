/* SPDX-License-Identifier:	GPL-2.0+ */

#ifndef __USB_CONTROLLER_H__
#define __USB_CONTROLLER_H__

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "reg/reg-usb.h"

#define USBC_MAX_OPEN_NUM 8
#define USBC_MAX_CTL_NUM 3
#define USBC_MAX_EP_NUM 6
#define USBC0_MAX_FIFO_SIZE (8 * 1024)
#define USBC_EP0_FIFOSIZE 64

typedef struct fifo_info {
	uint32_t port0_fifo_addr;
	uint32_t port0_fifo_size;
} fifo_info_t;

/* Structure to store hardware information for the current USB port */
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
 * Open the USB OTG controller.
 *
 * @param otg_no The OTG controller number.
 * @return Returns the status of the operation.
 */
uint32_t usb_controller_open_otg(uint32_t otg_no);

/**
 * Close the USB OTG controller.
 *
 * @param husb The handle to the USB controller.
 * @return Returns the status of the operation.
 */
int usb_controller_close_otg(uint64_t husb);

/**
 * Set the ID status for the USB controller.
 *
 * @param husb The handle to the USB controller.
 * @param id_type The type of ID status to set.
 */
void usb_controller_force_id_status(uint64_t husb, uint32_t id_type);

/**
 * Force the VBUS valid state for the USB controller.
 *
 * @param husb The handle to the USB controller.
 * @param vbus_type The type of VBUS state to force.
 */
void usb_controller_force_vbus_valid(uint64_t husb, uint32_t vbus_type);

/**
 * Enable the ID pull-up resistor for the USB controller.
 *
 * @param husb The handle to the USB controller.
 */
void usb_controller_id_pull_enable(uint64_t husb);

/**
 * Disable the ID pull-up resistor for the USB controller.
 *
 * @param husb The handle to the USB controller.
 */
void usb_controller_id_pull_disable(uint64_t husb);

/**
 * Enable the DP/DM pull-up resistors for the USB controller.
 *
 * @param husb The handle to the USB controller.
 */
void usb_controller_dpdm_pull_enable(uint64_t husb);

/**
 * Disable the DP/DM pull-up resistors for the USB controller.
 *
 * @param husb The handle to the USB controller.
 */
void usb_controller_dpdm_pull_disable(uint64_t husb);

/**
 * Disable all miscellaneous USB interrupts.
 *
 * @param husb The handle to the USB controller.
 */
void usb_controller_int_disable_usb_misc_all(uint64_t husb);

/**
 * Disable all endpoint-specific interrupts.
 *
 * @param husb The handle to the USB controller.
 * @param ep_type The type of endpoint to disable interrupts for.
 */
void usb_controller_int_disable_ep_all(uint64_t husb, uint32_t ep_type);

/**
 * Enable specific miscellaneous USB interrupts.
 *
 * @param husb The handle to the USB controller.
 * @param mask The interrupt mask to enable.
 */
void usb_controller_int_enable_usb_misc_uint(uint64_t husb, uint32_t mask);

/**
 * Disable specific miscellaneous USB interrupts.
 *
 * @param husb The handle to the USB controller.
 * @param mask The interrupt mask to enable.
 */
void usb_controller_int_disable_usb_misc_uint(uint64_t husb, uint32_t mask);

/**
 * Enable interrupts for a specific endpoint.
 *
 * @param husb The handle to the USB controller.
 * @param ep_type The type of endpoint to enable interrupts for.
 * @param ep_index The index of the endpoint to enable interrupts for.
 */
void usb_controller_int_enable_ep(uint64_t husb, uint32_t ep_type, uint32_t ep_index);

/**
 * Get the pending interrupt status for a specific endpoint.
 *
 * @param husb The handle to the USB controller.
 * @param ep_type The type of endpoint.
 * @return Returns the pending interrupt status.
 */
uint32_t usb_controller_int_ep_pending(uint64_t husb, uint32_t ep_type);

/**
 * Clear the pending interrupt flag for a specific endpoint.
 *
 * @param husb The handle to the USB controller.
 * @param ep_type The type of endpoint.
 * @param ep_index The index of the endpoint.
 */
void usb_controller_int_clear_ep_pending(uint64_t husb, uint32_t ep_type, uint8_t ep_index);

/**
 * Clear the pending interrupt flags for all endpoints of a specific type.
 *
 * @param husb The handle to the USB controller.
 * @param ep_type The type of endpoint.
 */
void usb_controller_int_clear_ep_pending_all(uint64_t husb, uint32_t ep_type);

/**
 * Get the pending miscellaneous interrupt status.
 *
 * @param husb The handle to the USB controller.
 * @return Returns the pending miscellaneous interrupt status.
 */
uint32_t usb_controller_int_misc_pending(uint64_t husb);

/**
 * Clear the pending miscellaneous interrupt flag.
 *
 * @param husb The handle to the USB controller.
 * @param mask The interrupt mask to clear.
 */
void usb_controller_int_clear_misc_pending(uint64_t husb, uint32_t mask);

/**
 * Clear the pending miscellaneous interrupt flags for all interrupts.
 *
 * @param husb The handle to the USB controller.
 */
void usb_controller_int_clear_misc_pending_all(uint64_t husb);

/**
 * Get the active endpoint for the USB controller.
 *
 * @param husb The handle to the USB controller.
 * @return Returns the active endpoint.
 */
uint32_t usb_controller_get_active_ep(uint64_t husb);

/**
 * Disable interrupts for a specific endpoint.
 *
 * @param husb The handle to the USB controller.
 * @param ep_type The type of endpoint.
 * @param ep_index The index of the endpoint.
 */
void usb_controller_int_disable_ep(uint64_t husb, uint32_t ep_type, uint8_t ep_index);

/**
 * Select the active endpoint for the USB controller.
 *
 * @param husb The handle to the USB controller.
 * @param ep_index The index of the endpoint to select as active.
 */
void usb_controller_select_active_ep(uint64_t husb, uint8_t ep_index);

/**
 * Configure the FIFO for a default transmit endpoint.
 *
 * @param addr The address of the USB controller.
 */
void usb_controller_config_fifo_tx_ep_default(uint32_t addr);

/**
 * Configure the FIFO for a transmit endpoint.
 *
 * @param addr The address of the USB controller.
 * @param is_double_fifo Whether the endpoint has double buffering enabled.
 * @param fifo_size The size of the FIFO.
 * @param fifo_addr The base address of the FIFO.
 */
void usb_controller_config_fifo_tx_ep(uint32_t addr, uint32_t is_double_fifo, uint32_t fifo_size, uint32_t fifo_addr);

/**
 * Configure the FIFO for a default receive endpoint.
 *
 * @param addr The address of the USB controller.
 */
void usb_controller_config_fifo_rx_ep_default(uint32_t addr);

/**
 * Configure the FIFO for a receive endpoint.
 *
 * @param addr The address of the USB controller.
 * @param is_double_fifo Whether the endpoint has double buffering enabled.
 * @param fifo_size The size of the FIFO.
 * @param fifo_addr The base address of the FIFO.
 */
void usb_controller_config_fifo_rx_ep(uint32_t addr, uint32_t is_double_fifo, uint32_t fifo_size, uint32_t fifo_addr);

/**
 * Configure the FIFO for a specific endpoint.
 *
 * @param husb The handle to the USB controller.
 * @param ep_type The type of endpoint.
 * @param is_double_fifo Whether the endpoint has double buffering enabled.
 * @param fifo_size The size of the FIFO.
 * @param fifo_addr The base address of the FIFO.
 */
void usb_controller_config_fifo(uint64_t husb, uint32_t ep_type, uint32_t is_double_fifo, uint32_t fifo_size, uint32_t fifo_addr);

/*
 * Function name: usb_controller_get_vbus_status
 *
 * @param husb: The USB controller handle. (Input)
 *
 * @return Returns the VBUS status value.
 *
 * Description:
 * This function gets the VBUS status of the USB controller.
 * It takes the USB controller handle as an input parameter.
 * The function first checks if the USB controller handle is not NULL.
 * It then reads the device control register to get the VBUS status.
 * The read value is shifted right by the VBUS bit position to get the VBUS status value.
 * The function uses a switch statement to check and return the corresponding VBUS status values.
 * If the VBUS status value is not recognized, it returns the default "below session end" status value.
 */
uint32_t usb_controller_get_vbus_status(uint64_t husb);

/*
 * Function name: usb_controller_read_len_from_fifo
 * 
 * @param husb: The USB controller handle. (Input)
 * @param ep_type: The endpoint type. (Input)
 * 
 * @return Returns the length of data available in the FIFO for the given endpoint type.
 *
 * Description:
 * This function reads the length of data available in the FIFO for the given endpoint type using the USB controller.
 * The function first checks if the USB controller handle is not NULL.
 * Then, the function reads the length from the appropriate register based on the endpoint type provided.
 * If the endpoint type is USBC_EP_TYPE_EP0, the count is read from the COUNT0 register.
 * If the endpoint type is USBC_EP_TYPE_TX, the count is 0 since this is an endpoint for transmitting data.
 * If the endpoint type is USBC_EP_TYPE_RX, the count is read from the RXCOUNT register.
 * Finally, the function returns the total length of data available in the FIFO for the given endpoint type.
 */
uint32_t usb_controller_read_len_from_fifo(uint64_t husb, uint32_t ep_type);

/*
 * Function name: usb_controller_write_packet
 * 
 * @param husb: The USB controller handle. (Input)
 * @param fifo: The FIFO number to write the packet to. (Input)
 * @param cnt: The number of bytes to write. (Input)
 * @param buff: The buffer containing the data to be written. (Input)
 * 
 * @return Returns the total number of bytes written.
 *
 * Description:
 * This function writes a packet of data to the specified FIFO using the USB controller.
 * The function first checks if the USB controller handle and the data buffer are not NULL.
 * Then, it adjusts the data by converting the buffer pointer to a 32-bit or 8-bit pointer based on the length.
 * The function then writes the data to the FIFO in chunks of 4 bytes (32-bit) until all 32-bit chunks are written.
 * After that, it processes the remaining non-4-byte part by writing 1 byte at a time.
 * Finally, the function returns the total number of bytes written.
 */
uint32_t usb_controller_write_packet(uint64_t husb, uint32_t fifo, uint32_t cnt, void *buff);

/*
 * Function name: usb_controller_read_packet
 * 
 * @param husb: The USB controller handle. (Input)
 * @param fifo: The FIFO number to read the packet from. (Input)
 * @param cnt: The number of bytes to read. (Input)
 * @param buff: The buffer to store the read data. (Output)
 * 
 * @return Returns the total number of bytes read.
 *
 * Description:
 * This function reads a packet of data from the specified FIFO using the USB controller.
 * The function first checks if the USB controller handle and the data buffer are not NULL.
 * Then, it adjusts the data by converting the buffer pointer to a 32-bit or 8-bit pointer based on the length.
 * The function then reads the data from the FIFO in chunks of 4 bytes (32-bit) until all 32-bit chunks are read.
 * After that, it processes the remaining non-4-byte part by reading 1 byte at a time.
 * Finally, the function returns the total number of bytes read.
 */
uint32_t usb_controller_read_packet(uint64_t husb, uint32_t fifo, uint32_t cnt, void *buff);

/*
 * Function name: usb_controller_config_fifo_base
 *
 * @param husb: The USB controller handle. (Input)
 * @param sram_base: The base address of the SRAM to be mapped to the USB FIFO. (Input)
 * @param fifo_mode: The mode of the FIFO. (Input)
 *
 * Description:
 * This function maps the SRAM region D to be used by the USB FIFO.
 * It takes the USB controller handle, the base address of the SRAM, and the FIFO mode as input parameters.
 * The function first checks if the USB controller handle is not NULL.
 * If the USB port number of the controller is 0, it sets the FIFO address and size for port 0.
 * The FIFO size is set to 8KB (8192 bytes).
 * Finally, the function returns.
 */
void usb_controller_config_fifo_base(uint64_t husb, uint32_t sram_base);

/*
 * Function name: usb_controller_get_port_fifo_start_addr
 *
 * @param husb: The USB controller handle. (Input)
 *
 * @return Returns the start address of the port FIFO.
 *
 * Description:
 * This function gets the start address of the port FIFO in the USB controller.
 * It takes the USB controller handle as an input parameter.
 * The function first checks if the USB controller handle is not NULL.
 * If the port number in the USB controller handle is 0, it returns the start address of port 0 FIFO.
 * If the port number is 1, it returns the start address of port 1 FIFO.
 * If the port number is neither 0 nor 1, it returns the start address of port 2 FIFO.
 */
uint32_t usb_controller_get_port_fifo_start_addr(uint64_t husb);

/*
 * Function name: usb_controller_get_port_fifo_size
 *
 * @param husb: The USB controller handle. (Input)
 *
 * @return Returns the size of the port FIFO.
 *
 * Description:
 * This function gets the size of the port FIFO in the USB controller.
 * It takes the USB controller handle as an input parameter.
 * The function first checks if the USB controller handle is not NULL.
 * If the port number in the USB controller handle is 0, it returns the size of port 0 FIFO.
 * Otherwise, it returns the size of port 1 FIFO. As same as port2 FIFO.
 */
uint32_t usb_controller_get_port_fifo_size(uint64_t husb);

/*
 * Function name: usb_controller_select_fifo
 *
 * @param husb: The USB controller handle. (Input)
 * @param ep_index: The endpoint index. (Input)
 *
 * @return Returns the FIFO address for the specified endpoint.
 *
 * Description:
 * This function selects the FIFO for the specified endpoint in the USB controller.
 * It takes the USB controller handle and the endpoint index as input parameters.
 * The function first checks if the USB controller handle is not NULL.
 * If it is NULL, it returns 0.
 * Otherwise, it calculates and returns the FIFO address for the specified endpoint.
 */
uint32_t usb_controller_select_fifo(uint64_t husb, uint32_t ep_index);

#endif// __USB_CONTROLLER_H__