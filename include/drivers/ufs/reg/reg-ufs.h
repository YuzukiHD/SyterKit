/* SPDX-License-Identifier:	GPL-2.0+ */

#ifndef __REG_UFS_H__
#define __REG_UFS_H__

/* UFSHCI Registers */
enum {
    REG_CONTROLLER_CAPABILITIES = 0x00,
    REG_UFS_VERSION = 0x08,
    REG_CONTROLLER_DEV_ID = 0x10,
    REG_CONTROLLER_PROD_ID = 0x14,
    REG_AUTO_HIBERNATE_IDLE_TIMER = 0x18,
    REG_INTERRUPT_STATUS = 0x20,
    REG_INTERRUPT_ENABLE = 0x24,
    REG_CONTROLLER_STATUS = 0x30,
    REG_CONTROLLER_ENABLE = 0x34,
    REG_UIC_ERROR_CODE_PHY_ADAPTER_LAYER = 0x38,
    REG_UIC_ERROR_CODE_DATA_LINK_LAYER = 0x3C,
    REG_UIC_ERROR_CODE_NETWORK_LAYER = 0x40,
    REG_UIC_ERROR_CODE_TRANSPORT_LAYER = 0x44,
    REG_UIC_ERROR_CODE_DME = 0x48,
    REG_UTP_TRANSFER_REQ_INT_AGG_CONTROL = 0x4C,
    REG_UTP_TRANSFER_REQ_LIST_BASE_L = 0x50,
    REG_UTP_TRANSFER_REQ_LIST_BASE_H = 0x54,
    REG_UTP_TRANSFER_REQ_DOOR_BELL = 0x58,
    REG_UTP_TRANSFER_REQ_LIST_CLEAR = 0x5C,
    REG_UTP_TRANSFER_REQ_LIST_RUN_STOP = 0x60,
    REG_UTP_TASK_REQ_LIST_BASE_L = 0x70,
    REG_UTP_TASK_REQ_LIST_BASE_H = 0x74,
    REG_UTP_TASK_REQ_DOOR_BELL = 0x78,
    REG_UTP_TASK_REQ_LIST_CLEAR = 0x7C,
    REG_UTP_TASK_REQ_LIST_RUN_STOP = 0x80,
    REG_UIC_COMMAND = 0x90,
    REG_UIC_COMMAND_ARG_1 = 0x94,
    REG_UIC_COMMAND_ARG_2 = 0x98,
    REG_UIC_COMMAND_ARG_3 = 0x9C,
    UFSHCI_REG_SPACE_SIZE = 0xA0,
    REG_UFS_CCAP = 0x100,
    REG_UFS_CRYPTOCAP = 0x104,
    REG_UFS_CFG = 0x200,
    REG_UFS_CLK_GATE = 0x204,
    REG_UFS_PD_PSW_DLY = 0x314,
    REG_UFS_PD_CTRL = 0x320,
    REG_UFS_PD_STAT = 0x324,
};

/* Controller capability masks */
enum {
    MASK_TRANSFER_REQUESTS_SLOTS = 0x0000001F,
    MASK_TASK_MANAGEMENT_REQUEST_SLOTS = 0x00070000,
    MASK_AUTO_HIBERN8_SUPPORT = 0x00800000,
    MASK_64_ADDRESSING_SUPPORT = 0x01000000,
    MASK_OUT_OF_ORDER_DATA_DELIVERY_SUPPORT = 0x02000000,
    MASK_UIC_DME_TEST_MODE_SUPPORT = 0x04000000,
};

#endif// __REG_UFS_H__