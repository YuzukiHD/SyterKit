/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVER_H__
#define __DRIVER_H__

#include <stdbool.h>

#include <initcall.h>

#ifdef __cplusplus
extern "C" {
#endif

struct device;
struct driver;

/** @brief Match a device against a driver-specific rule. */
typedef bool (*driver_match_t)(const struct device *device,
				      const struct driver *driver);

/** @brief Initialize a device selected for a driver. */
typedef int (*driver_probe_t)(struct device *device);

/** @brief Release a device before it is detached from its driver. */
typedef void (*driver_remove_t)(struct device *device);

/**
 * @brief Flat SyterKit driver descriptor.
 *
 * A driver may supply a custom match callback. Otherwise the core compares
 * compatible strings and falls back to names when no compatible is provided.
 */
struct driver {
	const char *name;
	const char *compatible;
	driver_match_t match;
	driver_probe_t probe;
	driver_remove_t remove;

	struct driver *__next;
	bool __registered;
};

/**
 * @brief Static hardware or logical device descriptor.
 *
 * Board code owns the descriptor and its platform data for the entire time the
 * device is registered. The core does not allocate or copy either object.
 */
struct device {
	const char *name;
	const char *compatible;
	void *platform_data;
	void *driver_data;
	struct driver *driver;

	struct device *__next;
	bool __registered;
};

/** @brief Successful driver-core operation. */
#define DRIVER_OK 0
/** @brief Invalid descriptor or operation. */
#define DRIVER_ERROR_INVALID (-1)
/** @brief Duplicate pointer or descriptor name. */
#define DRIVER_ERROR_EXISTS (-2)

/**
 * @brief Register a driver and probe matching unbound devices.
 * @param[in,out] driver Static driver descriptor.
 * @return Zero on success, a registration error, or the first probe error.
 *
 * The driver remains registered when probing a device fails.
 */
int driver_register(struct driver *driver);

/**
 * @brief Unregister a driver and re-probe devices detached from it.
 * @param[in,out] driver Registered driver descriptor.
 * @return Zero on success or a negative driver-core error.
 */
int driver_unregister(struct driver *driver);

/**
 * @brief Find a registered driver by name.
 * @param[in] name Driver name to find.
 * @return Registered driver, or null when no driver has that name.
 */
struct driver *driver_find(const char *name);

/**
 * @brief Register a device and bind the first matching driver.
 * @param[in,out] device Static device descriptor.
 * @return Zero on success, a registration error, or the first probe error.
 *
 * The device remains registered and unbound when every matching probe fails.
 */
int device_register(struct device *device);

/**
 * @brief Remove a device from the registry and detach its driver.
 * @param[in,out] device Registered device descriptor.
 * @return Zero on success or a negative driver-core error.
 */
int device_unregister(struct device *device);

/**
 * @brief Probe all registered drivers against an unbound device.
 * @param[in,out] device Registered device descriptor.
 * @return Zero when bound or no driver matches, otherwise the first probe error.
 */
int device_probe(struct device *device);

/**
 * @brief Find a registered device by name.
 * @param[in] name Device name to find.
 * @return Registered device, or null when no device has that name.
 */
struct device *device_find(const char *name);

/**
 * @brief Return whether the driver matches the device.
 * @param[in] driver Driver descriptor to test.
 * @param[in] device Device descriptor to test.
 * @return True when the custom or default matching rule succeeds.
 */
bool driver_matches_device(const struct driver *driver,
				   const struct device *device);

/**
 * @brief Return the board-owned configuration associated with a device.
 * @param[in] device Device descriptor.
 * @return Platform data pointer supplied by the board.
 */
static inline void *device_get_platform_data(const struct device *device) {
	return device->platform_data;
}

/**
 * @brief Store driver-private state on a device.
 * @param[in,out] device Device descriptor.
 * @param[in] data Driver-private state pointer.
 *
 * The core clears this pointer after a failed probe and whenever the device is
 * detached from a driver.
 */
static inline void device_set_driver_data(struct device *device, void *data) {
	device->driver_data = data;
}

/**
 * @brief Return driver-private state stored on a device.
 * @param[in] device Device descriptor.
 * @return Driver-private state pointer.
 */
static inline void *device_get_driver_data(const struct device *device) {
	return device->driver_data;
}

#define __define_builtin_driver(driver_name, level)                       \
	static int __register_driver_##driver_name(void) {                 \
		return driver_register(&(driver_name));                       \
	}                                                                    \
	level(__register_driver_##driver_name)

#define __define_builtin_device(device_name, level)                       \
	static int __register_device_##device_name(void) {                 \
		return device_register(&(device_name));                       \
	}                                                                    \
	level(__register_device_##device_name)

/* Built-in descriptors normally register at the device initcall level. */
#define builtin_driver(driver_name)                                      \
	__define_builtin_driver(driver_name, device_initcall)
#define builtin_device(device_name)                                      \
	__define_builtin_device(device_name, device_initcall)

/* Console-class devices may register before core services emit output. */
#define early_builtin_driver(driver_name)                                \
	__define_builtin_driver(driver_name, early_initcall)
#define early_builtin_device(device_name)                                \
	__define_builtin_device(device_name, early_initcall)

#ifdef __cplusplus
}
#endif

#endif /* __DRIVER_H__ */
