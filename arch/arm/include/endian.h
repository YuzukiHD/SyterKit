/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file endian.h
 * @brief Compile-time byte-order constants for ARM firmware.
 *
 * SyterKit builds little-endian by default. Defining @c __BIG_ENDIAN before
 * including this header selects the alternate byte order and updates
 * ::BYTE_ORDER accordingly.
 */

#ifndef __ENDIAN_H__
#define __ENDIAN_H__

#ifdef __cplusplus
extern "C" {
#endif

#define LITTLE_ENDIAN (0x1234)
#define BIG_ENDIAN (0x4321)

#if (!defined(__LITTLE_ENDIAN) && !defined(__BIG_ENDIAN))
#define __LITTLE_ENDIAN
#endif

#if defined(__LITTLE_ENDIAN)
#define BYTE_ORDER LITTLE_ENDIAN
#elif defined(__BIG_ENDIAN)
#define BYTE_ORDER BIG_ENDIAN
#else
#error "Unknown byte order!"
#endif

#ifdef __cplusplus
}
#endif

#endif /* __ENDIAN_H__ */
