/* SPDX-License-Identifier: MIT */

/**
 * @file driver.h
 * @brief dt2c driver-compatibility annotation header.
 *
 * Declares the marker macro that dt2c recognizes when it parses driver
 * sources. The macro expands to nothing and emits no target data.
 */

#ifndef __SYTERKIT_DT2C_DRIVER_H__
#define __SYTERKIT_DT2C_DRIVER_H__

/* Parsed by dt2c from driver sources; emits no target data. */
#define DT2C_DRIVER_COMPAT(compatible)

#endif /* __SYTERKIT_DT2C_DRIVER_H__ */
