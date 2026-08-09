/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __ARM32_LINKAGE_H__
#define __ARM32_LINKAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define ALIGN .align 0
#define ALIGN_STR ".align 0"

#define ENTRY(name) \
	.globl name;    \
	ALIGN;          \
	name:

#define WEAK(name) \
	.weak name;    \
	name:

#define END(name) .size name, .- name

#define ENDPROC(name)       \
	.type name, % function; \
	END(name)

#define ARMV7_USR_MODE 0x10
#define ARMV7_FIQ_MODE 0x11
#define ARMV7_IRQ_MODE 0x12
#define ARMV7_SVC_MODE 0x13
#define ARMV7_MON_MODE 0x16
#define ARMV7_ABT_MODE 0x17
#define ARMV7_UND_MODE 0x1b
#define ARMV7_SYSTEM_MODE 0x1f
#define ARMV7_MODE_MASK 0x1f
#define ARMV7_FIQ_MASK 0x40
#define ARMV7_IRQ_MASK 0x80

#ifdef __cplusplus
}
#endif

#endif /* __ARM32_LINKAGE_H__ */
