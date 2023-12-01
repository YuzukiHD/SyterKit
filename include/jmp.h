/* SPDX-License-Identifier: Apache-2.0 */

#ifndef _JMP_H_
#define _JMP_H_

void enable_kernel_smp(void);

void syterkit_jmp(uint32_t addr);

void syterkit_jmp_kernel(uint32_t addr, uint32_t fdt);

#endif // _JMP_H_