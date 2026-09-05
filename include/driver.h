/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file driver.h
 * @brief Common status values returned by SyterKit drivers.
 *
 * This header intentionally does not declare a driver/device registry.
 * SyterKit drivers are configured by the application and expose their
 * operations directly; the old registry lived in drivers/core.c and is no
 * longer part of the runtime model.
 */

#ifndef __DRIVER_H__
#define __DRIVER_H__

/*
 * Common status values used by the explicitly selected drivers.
 *
 * This header intentionally does not declare a driver/device registry.  SyterKit
 * drivers are configured by the application and expose their operations
 * directly; the old registry lived in drivers/core.c and is no longer part of
 * the runtime model.
 */
#define DRIVER_OK 0
#define DRIVER_ERROR_INVALID (-1)
#define DRIVER_ERROR_EXISTS (-2)

#endif /* __DRIVER_H__ */
