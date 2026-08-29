/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "ufs: " fmt

/**
 * @file ufs.c
 * @brief Top-level UFS device layer.
 *
 * Composes the UFS host controller and SCSI layers into a block-device API:
 * it performs the device-management handshake, selects the power mode, and
 * exposes read, write, and capacity helpers.  It adds no register or protocol
 * details of its own.
 */

/* Top-level UFS device layer.  It composes ufshc and SCSI without adding
 * register or protocol details of its own. */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <dt2c/driver.h>
#include <timer.h>

#include <drivers/ufs/host/sunxi.h>
#include <drivers/ufs/ufs.h>

/**
 * @brief Send a NOP command, retrying protocol errors.
 *
 * Stops immediately on a transport timeout to avoid spinning on a dead
 * controller.
 *
 * @param[in] host Host controller descriptor.
 * @return 0 on success, otherwise the last error.
 */
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
		pr_err("NOP handshake failed after %u attempts ret=%d\n", attempts, ret);
	return ret;
}

/**
 * @brief Set or read the fDeviceInit flag with retries.
 *
 * @param[in] host Host controller descriptor.
 * @param[in] set true to set the flag, false to read it.
 * @param[out] value Receives the flag value when reading.
 * @return 0 on success, otherwise the last error.
 */
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

/**
 * @brief Read or write the bRefClkFreq attribute with retries.
 *
 * @param[in] host Host controller descriptor.
 * @param[in,out] value Attribute value to write, or storage for the read.
 * @param[in] write true to write the attribute, false to read it.
 * @return 0 on success, otherwise the last error.
 */
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

/**
 * @brief Synchronize the device reference clock attribute.
 *
 * Reads the platform-selected reference clock frequency and programs the
 * device bRefClkFreq attribute to match when it differs.
 *
 * @param[in] host Host controller descriptor.
 * @return 0 on success, otherwise an error code.
 */
static int ufs_sync_device_ref_clk(struct ufshc_host *host)
{
	uint32_t target;
	uint32_t current;
	int ret;

	if (!host)
		return 0;
	ret = sunxi_ufs_get_ref_clk_freq(&target);
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

/**
 * @brief Select a power mode negotiated with the device.
 *
 * Queries the device maximum power mode and computes a HS or PWM mode with
 * matching gear and lane counts, preferring high speed with fallback to the
 * device PWM parameters.
 *
 * @param[in] host Host controller descriptor.
 * @param[out] selected Receives the selected power mode.
 * @return 0 on success, otherwise an error code.
 */
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
	host_prefers_hs = true;
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
		ret = sunxi_ufs_get_hs_rate(&hs_rate);
		if (ret || (hs_rate != UFSHC_HS_RATE_A && hs_rate != UFSHC_HS_RATE_B))
			return ret ? ret : UFSHC_ERR_IO;
		mode.hs_rate = (uint8_t)hs_rate;
	}
	*selected = mode;
	pr_info("selected mode pwr=%u/%u gear=%u/%u lane=%u/%u hs_rate=%u\n", mode.pwr_tx, mode.pwr_rx,
		mode.gear_tx, mode.gear_rx, mode.lane_tx, mode.lane_rx, mode.hs_rate);
	return 0;
}

/**
 * @brief Initialize one UFS logical unit.
 *
 * Initializes the host controller, completes the NOP and fDeviceInit
 * handshake, reads the device descriptor and power capabilities, transitions
 * the link to the selected power mode, and scans the logical unit.
 *
 * @param[out] device UFS device descriptor to initialize.
 * @param[in] config Host controller configuration.
 * @param[in] lun Logical unit number to attach.
 * @return 0 on success, otherwise an error code.
 */
int ufs_init_lun(struct ufs_device *device, const struct ufshc_config *config, uint8_t lun)
{
	uint64_t start;
	uint32_t timeout_us;
	bool device_init;
	uint16_t manufacturer_id = 0;
	struct ufshc_power_mode power_mode;
	int ret;

	if (!device || !config || lun > UFS_SCSI_MAX_LUN)
		return -1;
	pr_info("initialize LUN %u\n", lun);
	memset(device, 0, sizeof(*device));
	ret = ufshc_init(&device->host, config);
	if (ret) {
		pr_err("initialization failed ret=%d\n", ret);
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
	pr_info("fDeviceInit set, waiting for device\n");
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
	pr_info("device initialization complete\n");
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
		pr_err("unable to determine power mode ret=%d\n", ret);
		goto exit_host;
	}
	/* The device attribute must match the clock selected by the Sunxi host
	 * before the HS transition.  The native flow performs this after device
	 * initialization and descriptor discovery, when the attribute is usable. */
	ret = ufs_sync_device_ref_clk(&device->host);
	if (ret)
		/* Some devices expose the attribute only after their internal
		 * initialization has settled; keep the native best-effort behavior. */
		pr_warn("unable to synchronize bRefClkFreq (%d)\n", ret);
	ret = ufshc_change_power_mode(&device->host, &power_mode);
	if (ret) {
		pr_err("power mode transition failed ret=%d\n", ret);
		goto exit_host;
	}
	pr_info("power mode transition complete\n");

	ret = ufs_scsi_init(&device->scsi, &device->host, lun);
	if (ret) {
		pr_err("SCSI LUN initialization failed ret=%d\n", ret);
		goto exit_host;
	}
	device->scsi.manufacturer_id = manufacturer_id;
	device->lun = lun;
	device->initialized = true;
	return 0;

exit_host:
	pr_err("initialization of LUN %u aborted ret=%d\n", lun, ret);
	ufshc_exit(&device->host);
	return ret;
}

/**
 * @brief Initialize the UFS device on LUN 0.
 *
 * @param[out] device UFS device descriptor to initialize.
 * @param[in] config Host controller configuration.
 * @return 0 on success, otherwise an error code.
 */
int ufs_init(struct ufs_device *device, const struct ufshc_config *config)
{
	return ufs_init_lun(device, config, 0U);
}

/**
 * @brief Tear down the UFS device.
 *
 * @param[in,out] device UFS device descriptor to deinitialize.
 */
void ufs_exit(struct ufs_device *device)
{
	if (!device)
		return;
	ufshc_exit(&device->host);
	device->scsi.present = false;
	device->initialized = false;
}

/**
 * @brief Read blocks from the UFS device.
 *
 * @param[in] device Initialized UFS device descriptor.
 * @param[in] lba Starting logical block address.
 * @param[in] blocks Number of blocks to read.
 * @param[out] buffer Destination buffer.
 * @return 0 on success, -1 when not initialized, or the SCSI error.
 */
int ufs_read(struct ufs_device *device, uint64_t lba, uint32_t blocks, void *buffer)
{
	if (!device || !device->initialized)
		return -1;
	return ufs_scsi_read(&device->scsi, lba, blocks, buffer);
}

/**
 * @brief Write blocks to the UFS device.
 *
 * @param[in] device Initialized UFS device descriptor.
 * @param[in] lba Starting logical block address.
 * @param[in] blocks Number of blocks to write.
 * @param[in] buffer Source buffer.
 * @return 0 on success, -1 when not initialized, or the SCSI error.
 */
int ufs_write(struct ufs_device *device, uint64_t lba, uint32_t blocks, const void *buffer)
{
	if (!device || !device->initialized)
		return -1;
	return ufs_scsi_write(&device->scsi, lba, blocks, buffer);
}

/**
 * @brief Return the device capacity in blocks.
 *
 * @param[in] device UFS device descriptor.
 * @return The block count, or zero when not initialized.
 */
uint64_t ufs_capacity(const struct ufs_device *device)
{
	return device && device->initialized ? device->scsi.block_count : 0;
}

/**
 * @brief Return the device block size in bytes.
 *
 * @param[in] device UFS device descriptor.
 * @return The block size, or zero when not initialized.
 */
uint32_t ufs_block_size(const struct ufs_device *device)
{
	return device && device->initialized ? device->scsi.block_size : 0;
}

/**
 * @brief Return the device manufacturer ID.
 *
 * @param[in] device UFS device descriptor.
 * @return The manufacturer ID, or zero when not initialized.
 */
uint16_t ufs_manufacturer_id(const struct ufs_device *device)
{
	return device && device->initialized ? device->scsi.manufacturer_id : 0;
}

/**
 * @brief Block-layer read helper.
 *
 * @param[in] device UFS device descriptor.
 * @param[out] buffer Destination buffer.
 * @param[in] lba Starting logical block address.
 * @param[in] blocks Number of blocks to read.
 * @return The number of blocks read, or zero on failure.
 */
uint32_t ufs_blk_read(struct ufs_device *device, void *buffer, uint64_t lba, uint32_t blocks)
{
	return ufs_read(device, lba, blocks, buffer) ? 0U : blocks;
}

/**
 * @brief Block-layer write helper.
 *
 * @param[in] device UFS device descriptor.
 * @param[in] buffer Source buffer.
 * @param[in] lba Starting logical block address.
 * @param[in] blocks Number of blocks to write.
 * @return The number of blocks written, or zero on failure.
 */
uint32_t ufs_blk_write(struct ufs_device *device, const void *buffer, uint64_t lba, uint32_t blocks)
{
	return ufs_write(device, lba, blocks, buffer) ? 0U : blocks;
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-ufs");
