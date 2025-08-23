/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef _SYS_CLIC_H_
#define _SYS_CLIC_H_

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <common.h>
#include <log.h>

#include <reg-clic.h>
#include <reg-ncat.h>

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

#define CLIC_IRQ_NUM (186)

typedef enum irq_trigger_type { IRQ_TRIGGER_TYPE_LEVEL,
								IRQ_TRIGGER_TYPE_EDGE_RISING,
								IRQ_TRIGGER_TYPE_EDGE_FALLING,
								IRQ_TRIGGER_TYPE_EDGE_BOTH } irq_trigger_type_t;

typedef struct irq_controller {
	uint16_t id;
	uint16_t irq_cnt;
	uint16_t parent_id;
	uint16_t irq_id;
	uint64_t reg_base_addr;
} irq_controller_t;

/**
 * @brief Handles the IRQ
 * 
 */
void do_irq(uint64_t cause);

/**
 * @brief Initializes the interrupt mechanism
 * 
 * @return 0 on success, or an error code
 */
int arch_interrupt_init(void);

/**
 * @brief Exits the interrupt mechanism
 * 
 * @return 0 on success, or an error code
 */
int arch_interrupt_exit(void);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// _SYS_CLIC_H_