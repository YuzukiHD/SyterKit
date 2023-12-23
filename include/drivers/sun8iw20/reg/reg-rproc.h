/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __REG_RPROC_H__
#define __REG_RPROC_H__

#define RISCV_CFG_BASE (0x06010000)// Base address for RISC-V configuration
#define RISCV_STA_ADD_REG \
    (RISCV_CFG_BASE + 0x0204)// Register address for RISC-V start address

#endif// __REG_RPROC_H__