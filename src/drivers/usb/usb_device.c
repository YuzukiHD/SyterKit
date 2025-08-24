/* SPDX-License-Identifier:	GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <common.h>
#include <log.h>

#include <sys-clk.h>

#include "usb.h"
#include "usb_controller.h"
#include "usb_device.h"

void usb_device_set_address_default(uint64_t husb) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

	if (usbc_otg == NULL) {
		return;
	}

	writeb(0x00, USBC_REG_FADDR(usbc_otg->base_addr));
}

void usb_device_set_address(uint64_t husb, uint8_t address) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

	if (usbc_otg == NULL) {
		return;
	}

	writeb(address, USBC_REG_FADDR(usbc_otg->base_addr));
}

uint32_t usb_device_query_transfer_mode(uint64_t husb) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

	if (usbc_otg == NULL) {
		return USBC_TS_MODE_UNKOWN;
	}

	if (usb_get_bit8(USBC_BP_POWER_D_HIGH_SPEED_FLAG, USBC_REG_PCTL(usbc_otg->base_addr))) {
		return USBC_TS_MODE_HS;
	} else {
		return USBC_TS_MODE_FS;
	}
}

void usb_device_config_transfer_mode(uint64_t husb, uint8_t ts_type, uint8_t speed_mode) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

	if (usbc_otg == NULL) {
		return;
	}

	/* select transfer mode, default bulk */
	switch (ts_type) {
		case USBC_TS_TYPE_CTRL:
			usb_device_transfer_type_ctrl(usbc_otg->base_addr);
			break;

		case USBC_TS_TYPE_ISO:
			usb_device_transfer_type_iso(usbc_otg->base_addr);
			break;

		case USBC_TS_TYPE_INT:
			usb_device_transfer_type_int(usbc_otg->base_addr);
			break;

		case USBC_TS_TYPE_BULK:
			usb_device_transfer_type_bulk(usbc_otg->base_addr);
			break;

		default:
			usb_device_transfer_type_default(usbc_otg->base_addr);
	}

	/* select transfer speed, default disable */
	switch (speed_mode) {
		case USBC_TS_MODE_HS:
			usb_device_transfer_mode_hs(usbc_otg->base_addr);
			break;

		case USBC_TS_MODE_FS:
			usb_device_transfer_mode_fs(usbc_otg->base_addr);
			break;

		case USBC_TS_MODE_LS:
			usb_device_transfer_mode_ls(usbc_otg->base_addr);
			break;

		default:
			usb_device_transfer_mode_default(usbc_otg->base_addr);
	}
}

void usb_device_connect_switch(uint64_t husb, uint32_t is_on) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

	if (usbc_otg == NULL) {
		return;
	}

	if (is_on == USBC_DEVICE_SWITCH_ON) {
		usb_set_bit8(USBC_BP_POWER_D_SOFT_CONNECT, USBC_REG_PCTL(usbc_otg->base_addr));
	} else {
		usb_clear_bit8(USBC_BP_POWER_D_SOFT_CONNECT, USBC_REG_PCTL(usbc_otg->base_addr));
	}
}

uint32_t usb_device_query_power_status(uint64_t husb) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

	if (usbc_otg == NULL) {
		return 0;
	}

	return (readb(USBC_REG_PCTL(usbc_otg->base_addr)) & 0x0f);
}


int usb_device_config_ep(uint64_t husb, uint32_t ts_type, uint32_t ep_type, uint32_t is_double_fifo, uint32_t ep_maxpkt) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

	if (usbc_otg == NULL) {
		return -1;
	}

	switch (ep_type) {
		case USBC_EP_TYPE_EP0:
			usb_device_ep0_config_ep0(usbc_otg->base_addr);
			break;

		case USBC_EP_TYPE_TX:
			usb_device_tx_config_ep(usbc_otg->base_addr, ts_type, is_double_fifo, ep_maxpkt);
			break;

		case USBC_EP_TYPE_RX:
			usb_device_rx_config_ep(usbc_otg->base_addr, ts_type, is_double_fifo, ep_maxpkt);
			break;

		default:
			return -1;
	}

	return 0;
}

int usb_device_config_ep_default(uint64_t husb, uint32_t ep_type) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

	if (usbc_otg == NULL) {
		return -1;
	}

	switch (ep_type) {
		case USBC_EP_TYPE_EP0:
			usb_device_ep0_config_ep0_default(usbc_otg->base_addr);
			break;

		case USBC_EP_TYPE_TX:
			usb_device_tx_config_ep_default(usbc_otg->base_addr);
			break;

		case USBC_EP_TYPE_RX:
			usb_device_rx_config_ep_default(usbc_otg->base_addr);
			break;

		default:
			return -1;
	}

	return 0;
}

int usb_device_config_ep_dma(uint64_t husb, uint32_t ep_type) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

	if (usbc_otg == NULL) {
		return -1;
	}

	switch (ep_type) {
		case USBC_EP_TYPE_EP0:
			return -1;

		case USBC_EP_TYPE_TX:
			usb_device_tx_config_ep_dma(usbc_otg->base_addr);
			usb_device_config_dma_trans(usbc_otg->base_addr);
			break;

		case USBC_EP_TYPE_RX:
			usb_device_rx_config_ep_dma(usbc_otg->base_addr);
			usb_device_config_dma_trans(usbc_otg->base_addr);
			break;

		default:
			return -1;
	}

	return 0;
}

int usb_device_clear_ep_dma(uint64_t husb, uint32_t ep_type) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

	if (usbc_otg == NULL) {
		return -1;
	}

	switch (ep_type) {
		case USBC_EP_TYPE_EP0:
			return -1;

		case USBC_EP_TYPE_TX:
			usb_device_tx_clear_ep_dma(usbc_otg->base_addr);
			usb_device_clear_dma_trans(usbc_otg->base_addr);
			break;

		case USBC_EP_TYPE_RX:
			usb_device_rx_clear_ep_dma(usbc_otg->base_addr);
			usb_device_clear_dma_trans(usbc_otg->base_addr);
			break;

		default:
			return -1;
	}

	return 0;
}

int usb_device_get_ep_stall(uint64_t husb, uint32_t ep_type) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

	if (usbc_otg == NULL) {
		return -1;
	}

	switch (ep_type) {
		case USBC_EP_TYPE_EP0:
			usb_device_ep0_get_stall(usbc_otg->base_addr);
			break;

		case USBC_EP_TYPE_TX:
			usb_device_tx_get_ep_stall(usbc_otg->base_addr);
			break;

		case USBC_EP_TYPE_RX:
			usb_device_rx_get_ep_stall(usbc_otg->base_addr);
			break;

		default:
			return -1;
	}

	return 0;
}

int usb_device_ep_send_stall(uint64_t husb, uint32_t ep_type) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

	if (usbc_otg == NULL) {
		return -1;
	}

	switch (ep_type) {
		case USBC_EP_TYPE_EP0:
			usb_device_ep0_send_stall(usbc_otg->base_addr);
			break;

		case USBC_EP_TYPE_TX:
			usb_device_tx_send_stall(usbc_otg->base_addr);
			break;

		case USBC_EP_TYPE_RX:
			usb_device_rx_send_stall(usbc_otg->base_addr);
			break;

		default:
			return -1;
	}

	return 0;
}

int usb_device_ep_clear_stall(uint64_t husb, uint32_t ep_type) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

	if (usbc_otg == NULL) {
		return -1;
	}

	switch (ep_type) {
		case USBC_EP_TYPE_EP0:
			usb_device_ep0_clear_stall(usbc_otg->base_addr);
			break;

		case USBC_EP_TYPE_TX:
			usb_device_tx_clear_stall(usbc_otg->base_addr);
			break;

		case USBC_EP_TYPE_RX:
			usb_device_rx_clear_stall(usbc_otg->base_addr);
			break;

		default:
			return -1;
	}

	return 0;
}

uint32_t usb_device_ctrl_get_setup_end(uint64_t husb) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

	if (usbc_otg == NULL) {
		return 0;
	}

	return usb_device_ep0_get_setup_end(usbc_otg->base_addr);
}

void usb_device_ctrl_clear_setup_end(uint64_t husb) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

	if (usbc_otg == NULL) {
		return;
	}
	usb_device_ep0_clear_setup_end(usbc_otg->base_addr);
}

int usb_device_write_data_status(uint64_t husb, uint32_t ep_type, uint32_t complete) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

	if (usbc_otg == NULL) {
		return -1;
	}

	if (complete) {
		return usb_device_write_data_complete(usbc_otg->base_addr, ep_type);
	} else {
		return usb_device_write_data_half(usbc_otg->base_addr, ep_type);
	}
}

int usb_device_read_data_status(uint64_t husb, uint32_t ep_type, uint32_t complete) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

	if (usbc_otg == NULL) {
		return -1;
	}

	if (complete) {
		return usb_device_read_data_complete(usbc_otg->base_addr, ep_type);
	} else {
		return usb_device_read_data_half(usbc_otg->base_addr, ep_type);
	}
}

uint32_t usb_device_get_read_data_ready(uint64_t husb, uint32_t ep_type) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

	if (usbc_otg == NULL) {
		return 0;
	}

	switch (ep_type) {
		case USBC_EP_TYPE_EP0:
			return usb_device_ep0_get_read_data_ready(usbc_otg->base_addr);
			break;

		case USBC_EP_TYPE_TX:
			break;

		case USBC_EP_TYPE_RX:
			return usb_device_rx_get_read_data_ready(usbc_otg->base_addr);

		default:
			break;
	}

	return 0;
}

uint32_t usb_device_get_write_data_ready(uint64_t husb, uint32_t ep_type) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

	if (usbc_otg == NULL) {
		return 0;
	}

	switch (ep_type) {
		case USBC_EP_TYPE_EP0:
			return usb_device_ep0_get_write_data_ready(usbc_otg->base_addr);

		case USBC_EP_TYPE_TX:
			return usb_device_tx_get_write_data_ready(usbc_otg->base_addr);

		case USBC_EP_TYPE_RX:
			break;

		default:
			break;
	}

	return 0;
}

uint32_t usb_device_get_write_data_ready_fifo_empty(uint64_t husb, uint32_t ep_type) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

	if (usbc_otg == NULL) {
		return 0;
	}

	switch (ep_type) {
		case USBC_EP_TYPE_EP0:
			return usb_device_ep0_get_write_data_ready(usbc_otg->base_addr);

		case USBC_EP_TYPE_TX:
			return usb_device_tx_get_write_data_ready_fifo_empty(usbc_otg->base_addr);

		case USBC_EP_TYPE_RX:
			break;

		default:
			break;
	}

	return 0;
}

int usb_device_iso_update_enable(uint64_t husb) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

	if (usbc_otg == NULL) {
		return -1;
	}

	usb_device_transfer_type_iso(usbc_otg->base_addr);
	return 0;
}

void usb_device_flush_fifo(uint64_t husb, uint32_t ep_type) {
	usb_controller_otg_t *usbc_otg = (usb_controller_otg_t *) husb;

	if (usbc_otg == NULL) {
		return;
	}

	switch (ep_type) {
		case USBC_EP_TYPE_EP0:
			usb_device_ep0_flush_fifo(usbc_otg->base_addr);
			break;

		case USBC_EP_TYPE_TX:
			usb_device_tx_flush_fifo(usbc_otg->base_addr);
			break;

		case USBC_EP_TYPE_RX:
			usb_device_rx_flush_fifo(usbc_otg->base_addr);
			break;

		default:
			break;
	}
}
