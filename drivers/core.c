/* SPDX-License-Identifier: GPL-2.0+ */

#include <stddef.h>
#include <string.h>

#include <driver.h>

static struct driver *drivers;
static struct device *devices;

static bool valid_name(const char *name) {
	return name != NULL && name[0] != '\0';
}

static int bind_device(struct device *device, struct driver *driver) {
	int result;

	if (device->driver != NULL || !driver_matches_device(driver, device))
		return DRIVER_OK;

	device->driver_data = NULL;
	result = driver->probe(device);
	if (result == DRIVER_OK)
		device->driver = driver;
	else
		device->driver_data = NULL;
	return result;
}

bool driver_matches_device(const struct driver *driver,
				   const struct device *device) {
	if (driver == NULL || device == NULL)
		return false;
	if (driver->match != NULL)
		return driver->match(device, driver);
	if (driver->compatible != NULL && device->compatible != NULL)
		return strcmp(driver->compatible, device->compatible) == 0;
	if (driver->name == NULL || device->name == NULL)
		return false;
	return strcmp(driver->name, device->name) == 0;
}

struct driver *driver_find(const char *name) {
	struct driver *driver;

	if (!valid_name(name))
		return NULL;
	for (driver = drivers; driver != NULL; driver = driver->__next) {
		if (strcmp(driver->name, name) == 0)
			return driver;
	}
	return NULL;
}

struct device *device_find(const char *name) {
	struct device *device;

	if (!valid_name(name))
		return NULL;
	for (device = devices; device != NULL; device = device->__next) {
		if (strcmp(device->name, name) == 0)
			return device;
	}
	return NULL;
}

int driver_register(struct driver *driver) {
	struct driver **tail;
	struct device *device;
	int first_error = DRIVER_OK;

	if (driver == NULL || !valid_name(driver->name) || driver->probe == NULL)
		return DRIVER_ERROR_INVALID;
	if (driver->__registered || driver_find(driver->name) != NULL)
		return DRIVER_ERROR_EXISTS;

	driver->__next = NULL;
	driver->__registered = true;
	for (tail = &drivers; *tail != NULL; tail = &(*tail)->__next)
		;
	*tail = driver;

	for (device = devices; device != NULL; device = device->__next) {
		int result = bind_device(device, driver);

		if (result != DRIVER_OK && first_error == DRIVER_OK)
			first_error = result;
	}
	return first_error;
}

int device_probe(struct device *device) {
	struct driver *driver;
	int first_error = DRIVER_OK;

	if (device == NULL || !device->__registered)
		return DRIVER_ERROR_INVALID;
	if (device->driver != NULL)
		return DRIVER_OK;

	for (driver = drivers; driver != NULL; driver = driver->__next) {
		int result = bind_device(device, driver);

		if (device->driver != NULL)
			return DRIVER_OK;
		if (result != DRIVER_OK && first_error == DRIVER_OK)
			first_error = result;
	}
	return first_error;
}

int device_register(struct device *device) {
	struct device **tail;

	if (device == NULL || !valid_name(device->name))
		return DRIVER_ERROR_INVALID;
	if (device->__registered || device_find(device->name) != NULL)
		return DRIVER_ERROR_EXISTS;

	device->driver = NULL;
	device->driver_data = NULL;
	device->__next = NULL;
	device->__registered = true;
	for (tail = &devices; *tail != NULL; tail = &(*tail)->__next)
		;
	*tail = device;
	return device_probe(device);
}

int device_unregister(struct device *device) {
	struct device **link;

	if (device == NULL || !device->__registered)
		return DRIVER_ERROR_INVALID;

	for (link = &devices; *link != NULL; link = &(*link)->__next) {
		if (*link == device)
			break;
	}
	if (*link == NULL)
		return DRIVER_ERROR_INVALID;

	if (device->driver != NULL && device->driver->remove != NULL)
		device->driver->remove(device);
	*link = device->__next;
	device->driver = NULL;
	device->driver_data = NULL;
	device->__next = NULL;
	device->__registered = false;
	return DRIVER_OK;
}

int driver_unregister(struct driver *driver) {
	struct driver **link;
	struct device *device;

	if (driver == NULL || !driver->__registered)
		return DRIVER_ERROR_INVALID;

	for (link = &drivers; *link != NULL; link = &(*link)->__next) {
		if (*link == driver)
			break;
	}
	if (*link == NULL)
		return DRIVER_ERROR_INVALID;

	*link = driver->__next;
	driver->__next = NULL;
	driver->__registered = false;

	for (device = devices; device != NULL; device = device->__next) {
		if (device->driver != driver)
			continue;
		if (driver->remove != NULL)
			driver->remove(device);
		device->driver = NULL;
		device->driver_data = NULL;
		device_probe(device);
	}
	return DRIVER_OK;
}
