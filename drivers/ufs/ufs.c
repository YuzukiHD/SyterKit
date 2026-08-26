/* SPDX-License-Identifier: GPL-2.0+ */

/* Top-level UFS device layer.  It composes ufshc and SCSI without adding
 * register or protocol details of its own. */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <dt2c/driver.h>
#include <timer.h>

#include <drivers/ufs/host.h>
#include <drivers/ufs/ufs.h>

static int ufs_nop_retry(struct ufshc_host *host)
{
	int ret = UFSHC_ERR_IO;
	unsigned int attempts = 0;

	for (unsigned int retry = 0; retry < 10U; ++retry) {
		attempts = retry + 1U;
		ret = ufshc_nop(host);
		if (ret && retry == 0U)
			ufs_debug("UFS: NOP first attempt ret=%d; retrying\n", ret);
		/* Native startup retries protocol errors but stops immediately on a
		 * transport timeout; repeating a dead controller only adds delay. */
		if (!ret || ret == UFSHC_ERR_TIMEOUT)
			break;
	}
	if (ret)
		printk_error("UFS: NOP handshake failed after %u attempts ret=%d\n", attempts, ret);
	return ret;
}

static int ufs_query_flag_retry(struct ufshc_host *host, bool set, bool *value)
{
	int ret = UFSHC_ERR_IO;

	if (value)
		*value = false;

	for (unsigned int retry = 0; retry < 3U; ++retry) {
		ret = ufshc_query_flag(host, UFSHC_QUERY_FLAG_FDEVICE_INIT, set, value);
		ufs_debug("UFS: %s fDeviceInit attempt %u ret=%d", set ? "set" : "read", retry + 1U, ret);
		if (!ret)
			break;
	}
	ufs_debug(" value=%u\n", value ? *value : 0U);
	return ret;
}

static int ufs_query_attribute_retry(struct ufshc_host *host, uint32_t *value, bool write)
{
	int ret = UFSHC_ERR_IO;

	if (value)
		*value = 0;

	for (unsigned int retry = 0; retry < 3U; ++retry) {
		ret = ufshc_query_attribute(host, UFSHC_QUERY_ATTR_REF_CLK_FREQ, 0, 0, value, write);
		ufs_debug("UFS: %s bRefClkFreq attempt %u ret=%d", write ? "write" : "read", retry + 1U, ret);
		if (!ret)
			break;
	}
	ufs_debug(" value=%u\n", value ? *value : 0U);
	return ret;
}

static int ufs_sync_device_ref_clk(struct ufshc_host *host)
{
	uint32_t target;
	uint32_t current;
	int ret;

	if (!host || !host->platform || !host->platform->get_ref_clk_freq)
		return 0;
	ret = host->platform->get_ref_clk_freq(host->platform->priv, &target);
	if (ret || target > 3U)
		return ret ? ret : UFSHC_ERR_INVALID;
	ufs_debug("UFS: device bRefClkFreq target=%u\n", target);
	ret = ufs_query_attribute_retry(host, &current, false);
	if (ret)
		return ret;
	ufs_debug("UFS: device bRefClkFreq current=%u\n", current);
	if (current == target)
		return 0;
	ret = ufs_query_attribute_retry(host, &target, true);
	return ret;
}

static int ufs_select_power_mode(struct ufshc_host *host, struct ufshc_power_mode *selected)
{
	struct ufshc_power_mode device_mode;
	struct ufshc_power_mode mode;
	uint32_t min_device_gear;
	uint32_t max_host_gear;
	uint32_t hs_rate;
	bool device_supports_hs;
	bool host_prefers_hs;
	int ret;

	if (!host || !selected)
		return UFSHC_ERR_INVALID;
	ret = ufshc_get_max_power_mode(host, &device_mode);
	if (ret)
		return ret;
	ufs_debug("UFS: device mode pwr=%u/%u gear=%u/%u lane=%u/%u\n", device_mode.pwr_tx, device_mode.pwr_rx,
		device_mode.gear_tx, device_mode.gear_rx, device_mode.lane_tx, device_mode.lane_rx);

	/* This is ufshcd_get_pwr_dev_param() expressed in the small boot-time
	 * representation.  A Sunxi host prefers HS, but the native config path
	 * falls back to the device's PWM parameters when the device has no HS
	 * capability. */
	device_supports_hs = device_mode.pwr_rx == UFSHC_PWR_FAST;
	host_prefers_hs = host->platform && host->platform->get_hs_rate;
	if (host_prefers_hs && !device_supports_hs)
		host_prefers_hs = false;
	memset(&mode, 0, sizeof(mode));
	mode.pwr_rx = mode.pwr_tx = (host_prefers_hs || device_supports_hs) ? UFSHC_PWR_FAST : UFSHC_PWR_SLOW;
	mode.lane_rx = device_mode.lane_rx > 2U ? 2U : device_mode.lane_rx;
	mode.lane_tx = device_mode.lane_tx > 2U ? 2U : device_mode.lane_tx;
	min_device_gear = device_mode.gear_rx < device_mode.gear_tx ? device_mode.gear_rx : device_mode.gear_tx;
	max_host_gear = host_prefers_hs ? 4U : min_device_gear;
	mode.gear_rx = min_device_gear < max_host_gear ? min_device_gear : max_host_gear;
	mode.gear_tx = mode.gear_rx;
	if (!mode.lane_rx || !mode.lane_tx || !mode.gear_rx || !mode.gear_tx)
		return UFSHC_ERR_IO;

	if (mode.pwr_rx == UFSHC_PWR_FAST || mode.pwr_tx == UFSHC_PWR_FAST) {
		hs_rate = UFSHC_HS_RATE_B;
		if (host->platform && host->platform->get_hs_rate) {
			ret = host->platform->get_hs_rate(host->platform->priv, &hs_rate);
			if (ret || (hs_rate != UFSHC_HS_RATE_A && hs_rate != UFSHC_HS_RATE_B))
				return ret ? ret : UFSHC_ERR_IO;
		}
		mode.hs_rate = (uint8_t)hs_rate;
	}
	*selected = mode;
	printk_info("UFS: selected mode pwr=%u/%u gear=%u/%u lane=%u/%u hs_rate=%u\n", mode.pwr_tx, mode.pwr_rx,
		mode.gear_tx, mode.gear_rx, mode.lane_tx, mode.lane_rx, mode.hs_rate);
	return 0;
}

int ufs_init_lun(struct ufs_device *device, const struct ufshc_config *config, uint8_t lun)
{
	struct ufshc_config selected_config;
	uint64_t start;
	uint32_t timeout_us;
	bool device_init;
	uint16_t manufacturer_id = 0;
	struct ufshc_power_mode power_mode;
	int ret;

	if (!device || !config || lun > UFS_SCSI_MAX_LUN)
		return -1;
	printk_info("UFS: initialize LUN %u\n", lun);
	memset(device, 0, sizeof(*device));
	selected_config = *config;
	if (!selected_config.platform)
		selected_config.platform = ufs_platform_default();
	ret = ufshc_init(&device->host, &selected_config);
	if (ret) {
		printk_error("UFS: UFSHCI initialization failed ret=%d\n", ret);
		return ret;
	}
	/* Complete the mandatory device-management handshake before issuing any
	 * SCSI command.  The device clears fDeviceInit when it has finished its
	 * internal initialization. */
	ret = ufs_nop_retry(&device->host);
	if (ret)
		goto exit_host;
	ret = ufs_query_flag_retry(&device->host, true, NULL);
	if (ret)
		goto exit_host;
	printk_info("UFS: fDeviceInit set, waiting for device\n");
	timeout_us = device->host.timeout_us ? device->host.timeout_us : UFSHC_TIMEOUT_US;
	start = time_us();
	for (;;) {
		ret = ufs_query_flag_retry(&device->host, false, &device_init);
		if (ret || !device_init)
			break;
		if (time_us() - start >= timeout_us) {
			ret = UFSHC_ERR_TIMEOUT;
			break;
		}
		mdelay(1);
	}
	if (ret)
		goto exit_host;
	printk_info("UFS: device initialization complete\n");
	/* Read the device descriptor at the same point as the native startup
	 * flow, before the SCSI scan.  Descriptor support is optional, so a
	 * failed read does not prevent block access. */
	{
		uint8_t descriptor[0x40];
		size_t descriptor_len = sizeof(descriptor);
		int descriptor_ret = UFSHC_ERR_IO;

		for (unsigned int retry = 0; retry < 3U; ++retry) {
			descriptor_len = sizeof(descriptor);
			descriptor_ret = ufshc_query_descriptor(
				&device->host, 0x00U, 0, 0, descriptor, sizeof(descriptor), &descriptor_len);
			if (!descriptor_ret)
				break;
		}
		if (!descriptor_ret && descriptor_len > 0x19U)
			manufacturer_id = ((uint16_t)descriptor[0x18] << 8) | descriptor[0x19];
		ufs_debug("UFS: device descriptor ret=%d length=%u manufacturer=0x%04x\n", descriptor_ret,
			(unsigned int)descriptor_len, manufacturer_id);
	}
	/* Read the device capabilities before changing bRefClkFreq, matching the
	 * native ufs_start() sequence.  The selected mode is applied only after
	 * the device reference-clock attribute has been synchronized. */
	ret = ufs_select_power_mode(&device->host, &power_mode);
	if (ret) {
		printk_error("UFS: unable to determine power mode ret=%d\n", ret);
		goto exit_host;
	}
	/* The device attribute must match the clock selected by the Sunxi host
	 * before the HS transition.  The native flow performs this after device
	 * initialization and descriptor discovery, when the attribute is usable. */
	ret = ufs_sync_device_ref_clk(&device->host);
	if (ret)
		/* Some devices expose the attribute only after their internal
		 * initialization has settled; keep the native best-effort behavior. */
		printk_warning("UFS: unable to synchronize bRefClkFreq (%d)\n", ret);
	ret = ufshc_change_power_mode(&device->host, &power_mode);
	if (ret) {
		printk_error("UFS: power mode transition failed ret=%d\n", ret);
		goto exit_host;
	}
	printk_info("UFS: power mode transition complete\n");

	ret = ufs_scsi_init(&device->scsi, &device->host, lun);
	if (ret) {
		printk_error("UFS: SCSI LUN initialization failed ret=%d\n", ret);
		goto exit_host;
	}
	device->scsi.manufacturer_id = manufacturer_id;
	device->lun = lun;
	device->initialized = true;
	return 0;

exit_host:
	printk_error("UFS: initialization of LUN %u aborted ret=%d\n", lun, ret);
	ufshc_exit(&device->host);
	return ret;
}

int ufs_init(struct ufs_device *device, const struct ufshc_config *config)
{
	return ufs_init_lun(device, config, 0U);
}

void ufs_exit(struct ufs_device *device)
{
	if (!device)
		return;
	ufshc_exit(&device->host);
	device->scsi.present = false;
	device->initialized = false;
}

int ufs_read(struct ufs_device *device, uint64_t lba, uint32_t blocks, void *buffer)
{
	if (!device || !device->initialized)
		return -1;
	return ufs_scsi_read(&device->scsi, lba, blocks, buffer);
}

int ufs_write(struct ufs_device *device, uint64_t lba, uint32_t blocks, const void *buffer)
{
	if (!device || !device->initialized)
		return -1;
	return ufs_scsi_write(&device->scsi, lba, blocks, buffer);
}

uint64_t ufs_capacity(const struct ufs_device *device)
{
	return device && device->initialized ? device->scsi.block_count : 0;
}

uint32_t ufs_block_size(const struct ufs_device *device)
{
	return device && device->initialized ? device->scsi.block_size : 0;
}

uint16_t ufs_manufacturer_id(const struct ufs_device *device)
{
	return device && device->initialized ? device->scsi.manufacturer_id : 0;
}

uint32_t ufs_blk_read(struct ufs_device *device, void *buffer, uint64_t lba, uint32_t blocks)
{
	return ufs_read(device, lba, blocks, buffer) ? 0U : blocks;
}

uint32_t ufs_blk_write(struct ufs_device *device, const void *buffer, uint64_t lba, uint32_t blocks)
{
	return ufs_write(device, lba, blocks, buffer) ? 0U : blocks;
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-ufs");
