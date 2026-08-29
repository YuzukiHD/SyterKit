/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file reg-usb.h
 * @brief Register map for the USB host-presence detector.
 */

#ifndef __REG_USB_H__
#define __REG_USB_H__

/* Registers used by the USB host-presence detector. */
#define USBC_REG_o_PCTL 0x0040
#define USBC_REG_o_VEND0 0x0043
#define USBC_REG_o_INTTxE 0x0048
#define USBC_REG_o_INTRxE 0x004a
#define USBC_REG_o_INTUSB 0x004c
#define USBC_REG_o_INTUSBE 0x0050
#define USBC_REG_o_ISCR 0x0400
#define USBC_REG_o_PHYCTL 0x0410

#define USBC_REG_PCTL(base) ((base) + USBC_REG_o_PCTL)
#define USBC_REG_VEND0(base) ((base) + USBC_REG_o_VEND0)
#define USBC_REG_INTTxE(base) ((base) + USBC_REG_o_INTTxE)
#define USBC_REG_INTRxE(base) ((base) + USBC_REG_o_INTRxE)
#define USBC_REG_INTUSB(base) ((base) + USBC_REG_o_INTUSB)
#define USBC_REG_INTUSBE(base) ((base) + USBC_REG_o_INTUSBE)
#define USBC_REG_ISCR(base) ((base) + USBC_REG_o_ISCR)

#define USBC_BP_POWER_D_ISO_UPDATE_EN 7
#define USBC_BP_POWER_D_SOFT_CONNECT 6
#define USBC_BP_POWER_D_HIGH_SPEED_EN 5

#define USBC_BP_INTUSB_DISCONNECT 5
#define USBC_BP_INTUSB_RESET 2

#define USBC_BP_ISCR_ID_PULLUP_EN 17
#define USBC_BP_ISCR_DPDM_PULLUP_EN 16
#define USBC_BP_ISCR_FORCE_ID 14
#define USBC_BP_ISCR_FORCE_VBUS_VALID 12
#define USBC_BP_ISCR_VBUS_CHANGE_DETECT 6
#define USBC_BP_ISCR_ID_CHANGE_DETECT 5
#define USBC_BP_ISCR_DPDM_CHANGE_DETECT 4

#define USBC_DEVICE_SWITCH_OFF 0
#define USBC_DEVICE_SWITCH_ON 1

#define USBC_INTUSB_DISCONNECT (1U << USBC_BP_INTUSB_DISCONNECT)
#define USBC_INTUSB_RESET (1U << USBC_BP_INTUSB_RESET)

#define USBC_PHY_CTL_VBUSVLDEXT 5
#define USBC_PHY_CTL_SIDDQ 3

#endif /* __REG_USB_H__ */
