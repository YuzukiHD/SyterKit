/* SPDX-License-Identifier: GPL-2.0+ */

#include <driver.h>

#include "syter_test.h"

struct probe_state {
	int result;
	unsigned int probes;
	unsigned int removes;
};

static int test_probe(struct device *device) {
	struct probe_state *state = device_get_platform_data(device);

	state->probes++;
	device_set_driver_data(device, state);
	return state->result;
}

static void test_remove(struct device *device) {
	struct probe_state *state = device_get_platform_data(device);

	state->removes++;
}

void test_case_main(const char *case_dir) {
	struct probe_state state = {.result = -7};
	struct probe_state name_state = {.result = 0};
	struct driver rejected = {
			.name = "rejected",
			.compatible = "test,device",
			.probe = test_probe,
			.remove = test_remove,
	};
	struct driver accepted = {
			.name = "accepted",
			.compatible = "test,device",
			.probe = test_probe,
			.remove = test_remove,
	};
	struct driver duplicate = {
			.name = "accepted",
			.compatible = "test,other",
			.probe = test_probe,
	};
	struct driver unrelated = {
			.name = "unrelated",
			.compatible = "test,other",
			.probe = test_probe,
	};
	struct device device = {
			.name = "device0",
			.compatible = "test,device",
			.platform_data = &state,
	};
	struct device duplicate_device = {
			.name = "device0",
			.compatible = "test,other",
	};
	struct driver name_driver = {
			.name = "name-only",
			.probe = test_probe,
			.remove = test_remove,
	};
	struct device name_device = {
			.name = "name-only",
			.platform_data = &name_state,
	};

	(void) case_dir;

	TEST_EQ(DRIVER_OK, device_register(&device));
	TEST_ASSERT(device.driver == NULL);
	TEST_ASSERT(device_find("device0") == &device);
	TEST_EQ(DRIVER_ERROR_EXISTS, device_register(&duplicate_device));

	TEST_EQ(-7, driver_register(&rejected));
	TEST_EQ(1U, state.probes);
	TEST_ASSERT(device.driver == NULL);
	TEST_ASSERT(device_get_driver_data(&device) == NULL);
	TEST_EQ(DRIVER_OK, driver_register(&unrelated));
	TEST_EQ(DRIVER_OK, driver_unregister(&unrelated));
	TEST_EQ(1U, state.probes);

	state.result = 0;
	TEST_EQ(DRIVER_OK, driver_register(&accepted));
	TEST_ASSERT(device.driver == &accepted);
	TEST_ASSERT(driver_find("accepted") == &accepted);
	TEST_ASSERT(device_get_driver_data(&device) == &state);
	TEST_EQ(DRIVER_ERROR_EXISTS, driver_register(&duplicate));

	state.result = -7;
	TEST_EQ(DRIVER_OK, driver_unregister(&accepted));
	TEST_EQ(1U, state.removes);
	TEST_ASSERT(device.driver == NULL);
	TEST_ASSERT(device_get_driver_data(&device) == NULL);

	state.result = 0;
	TEST_EQ(DRIVER_OK, device_probe(&device));
	TEST_ASSERT(device.driver == &rejected);
	TEST_EQ(DRIVER_OK, device_unregister(&device));
	TEST_EQ(2U, state.removes);
	TEST_ASSERT(device_get_driver_data(&device) == NULL);
	TEST_EQ(DRIVER_OK, driver_unregister(&rejected));

	TEST_EQ(DRIVER_OK, driver_register(&name_driver));
	TEST_EQ(DRIVER_OK, device_register(&name_device));
	TEST_ASSERT(name_device.driver == &name_driver);
	TEST_EQ(1U, name_state.probes);
	TEST_EQ(DRIVER_OK, device_unregister(&name_device));
	TEST_EQ(1U, name_state.removes);
	TEST_ASSERT(device_get_driver_data(&name_device) == NULL);
	TEST_EQ(DRIVER_OK, driver_unregister(&name_driver));
}
