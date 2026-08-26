/* SPDX-License-Identifier: GPL-2.0+ */

/*
 * UFS host-controller layer.
 *
 * This file deliberately knows nothing about SCSI commands.  It owns the
 * UFSHCI register programming, UIC command path and one-slot UTP transport;
 * the SCSI layer builds protocol requests on top of ufshc_exec().
 */
/* Keep this implementation freestanding: no libc or OS block layer is used. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <cache.h>
#include <io.h>
#include <limits.h>
#include <log.h>
#include <timer.h>

#include <drivers/ufs/ufshc.h>

#define UFS_ERR_INVALID UFSHC_ERR_INVALID
#define UFS_ERR_IO	UFSHC_ERR_IO
#define UFS_ERR_TIMEOUT UFSHC_ERR_TIMEOUT

/* Keep UTP descriptors in SRAM.  This mirrors the reference driver's pool:
 * UTRD at +0x000, UCD at +0x080, and a separately aligned UTMRD at +0x800. */
#define UFSHC_DMA_POOL_SIZE    2560U
#define UFSHC_DMA_UTRD_OFFSET  0U
#define UFSHC_DMA_UCD_OFFSET   128U
#define UFSHC_DMA_UTMRD_OFFSET 2048U

static uint8_t ufshc_dma_pool[UFSHC_DMA_POOL_SIZE] __attribute__((aligned(1024)));
static bool ufshc_devman_ocs_reported;

static inline struct ufshc_request_desc *ufshc_utrd(void)
{
	return (struct ufshc_request_desc *)(void *)(ufshc_dma_pool + UFSHC_DMA_UTRD_OFFSET);
}

static inline struct ufshc_command_desc *ufshc_ucd(void)
{
	return (struct ufshc_command_desc *)(void *)(ufshc_dma_pool + UFSHC_DMA_UCD_OFFSET);
}

static inline struct ufshc_task_request_desc *ufshc_utmrd(void)
{
	return (struct ufshc_task_request_desc *)(void *)(ufshc_dma_pool + UFSHC_DMA_UTMRD_OFFSET);
}

/* UPIU transaction and command-set values are encoded big endian. */
#define UPIU_FLAG_READ	0x40U
#define UPIU_FLAG_WRITE 0x20U

static inline uint32_t ufs_be32(uint32_t value)
{
	return __builtin_bswap32(value);
}

static uint32_t ufs_load32(const void *address)
{
	uint32_t value;

	memcpy(&value, address, sizeof(value));
	return value;
}

static void ufs_store32(void *address, uint32_t value)
{
	memcpy(address, &value, sizeof(value));
}

static inline uint32_t ufshc_read(const struct ufshc_host *host, uint32_t offset)
{
	return readl(host->base + offset);
}

static inline void ufshc_write(const struct ufshc_host *host, uint32_t offset, uint32_t value)
{
	writel(value, host->base + offset);
}

static uint32_t ufshc_timeout(const struct ufshc_host *host)
{
	return host->timeout_us ? host->timeout_us : UFSHC_TIMEOUT_US;
}

#ifdef CONFIG_DRIVER_UFS_DEBUG
static void ufshc_log_state(const struct ufshc_host *host, const char *stage)
{
	if (!host || !host->base)
		return;
	printk_info("UFSHCI: diag %s: hcs=%08x hce=%08x is=%08x ie=%08x utrl=%08x:%08x/%08x "
		    "utmr=%08x:%08x/%08x uic=%08x err=%08x/%08x/%08x/%08x/%08x\n",
		stage, ufshc_read(host, UFSHC_REG_CONTROLLER_STATUS), ufshc_read(host, UFSHC_REG_CONTROLLER_ENABLE),
		ufshc_read(host, UFSHC_REG_INTERRUPT_STATUS), ufshc_read(host, UFSHC_REG_INTERRUPT_ENABLE),
		ufshc_read(host, UFSHC_REG_UTRL_BASE_H), ufshc_read(host, UFSHC_REG_UTRL_BASE_L),
		ufshc_read(host, UFSHC_REG_UTRL_RUN_STOP), ufshc_read(host, UFSHC_REG_UTMRL_BASE_H),
		ufshc_read(host, UFSHC_REG_UTMRL_BASE_L), ufshc_read(host, UFSHC_REG_UTMRL_RUN_STOP),
		ufshc_read(host, UFSHC_REG_UIC_COMMAND), ufshc_read(host, UFSHC_REG_UIC_ERROR_PHY_ADAPTER),
		ufshc_read(host, UFSHC_REG_UIC_ERROR_DATA_LINK), ufshc_read(host, UFSHC_REG_UIC_ERROR_NETWORK),
		ufshc_read(host, UFSHC_REG_UIC_ERROR_TRANSPORT), ufshc_read(host, UFSHC_REG_UIC_ERROR_DME));
}
#else
#define ufshc_log_state(host, stage) do { } while (0)
#endif

static int ufshc_fail(struct ufshc_host *host, const char *stage, int ret)
{
	printk_error("UFSHCI: %s failed ret=%d\n", stage, ret);
	ufshc_log_state(host, stage);
	return ret;
}

static uint32_t ufshc_hci_version(const struct ufshc_host *host)
{
	uint32_t version = host->version;

	/* UFSHCI 1.x encodes the major/minor fields differently from 2.x+. */
	if (version & 0x00010000U)
		version = 0x100U | ((version & 0x00000100U) ? 0x10U : 0U);
	return version;
}

static uint32_t ufshc_cmd_type(const struct ufshc_host *host, uint32_t legacy_type)
{
	return ufshc_hci_version(host) <= 0x110U ? legacy_type : UFSHC_REQ_CMD_TYPE_UFS_STORAGE;
}

static int ufshc_wait_mask(const struct ufshc_host *host, uint32_t offset, uint32_t mask, uint32_t value)
{
	uint64_t start = time_us();

	while ((ufshc_read(host, offset) & mask) != value) {
		if (time_us() - start >= ufshc_timeout(host))
			return UFS_ERR_TIMEOUT;
	}
	return 0;
}

static void ufshc_clear_interrupts(struct ufshc_host *host)
{
	uint32_t status = ufshc_read(host, UFSHC_REG_INTERRUPT_STATUS);

	if (status)
		ufshc_write(host, UFSHC_REG_INTERRUPT_STATUS, status);
}

static void ufshc_abort_transfer(struct ufshc_host *host)
{
	/* UTRLCLR is a write-one-to-clear slot bitmap.  This implementation owns
	 * slot 0 only, so do not disturb any other request slots. */
	ufshc_write(host, UFSHC_REG_UTRL_LIST_CLEAR, 1U);
	ufshc_wait_mask(host, UFSHC_REG_UTRL_DOOR_BELL, 1U, 0U);
}

static void ufshc_sync_for_device(struct ufshc_host *host, const void *data, size_t data_len)
{
	struct ufshc_request_desc *utrd = ufshc_utrd();
	struct ufshc_task_request_desc *utmrd = ufshc_utmrd();
	struct ufshc_command_desc *ucd = ufshc_ucd();

	flush_dcache_range((uint64_t)(uintptr_t)utrd, (uint64_t)(uintptr_t)utrd + sizeof(*utrd));
	flush_dcache_range((uint64_t)(uintptr_t)utmrd, (uint64_t)(uintptr_t)utmrd + sizeof(*utmrd));
	flush_dcache_range((uint64_t)(uintptr_t)ucd, (uint64_t)(uintptr_t)ucd + sizeof(*ucd));
	if (data && data_len)
		flush_dcache_range((uint64_t)(uintptr_t)data, (uint64_t)(uintptr_t)data + data_len);
	data_sync_barrier();
}

static void ufshc_sync_for_cpu(struct ufshc_host *host, const void *data, size_t data_len)
{
	struct ufshc_request_desc *utrd = ufshc_utrd();
	struct ufshc_task_request_desc *utmrd = ufshc_utmrd();
	struct ufshc_command_desc *ucd = ufshc_ucd();

	invalidate_dcache_range((uint64_t)(uintptr_t)utrd, (uint64_t)(uintptr_t)utrd + sizeof(*utrd));
	invalidate_dcache_range((uint64_t)(uintptr_t)utmrd, (uint64_t)(uintptr_t)utmrd + sizeof(*utmrd));
	invalidate_dcache_range((uint64_t)(uintptr_t)ucd, (uint64_t)(uintptr_t)ucd + sizeof(*ucd));
	if (data && data_len)
		invalidate_dcache_range((uint64_t)(uintptr_t)data, (uint64_t)(uintptr_t)data + data_len);
	data_sync_barrier();
}

static int ufshc_enable_controller(struct ufshc_host *host)
{
	uint64_t start;

	/* HCE is a self-clearing initialization request on UFSHCI. */
	ufshc_write(host, UFSHC_REG_CONTROLLER_ENABLE, UFSHC_HCE);
	start = time_us();
	while (!(ufshc_read(host, UFSHC_REG_CONTROLLER_ENABLE) & UFSHC_HCE)) {
		if (time_us() - start >= ufshc_timeout(host))
			return UFS_ERR_TIMEOUT;
	}

	return 0;
}

static void ufshc_configure_slot(struct ufshc_host *host);

static int ufshc_reinitialize_controller(struct ufshc_host *host)
{
	int ret;

	/* Native LINK STARTUP retries re-enter HCE initialization after a failed
	 * UIC exchange.  Recreate that boundary without dropping platform clocks. */
	ufshc_write(host, UFSHC_REG_CONTROLLER_ENABLE, 0U);
	ret = ufshc_wait_mask(host, UFSHC_REG_CONTROLLER_ENABLE, UFSHC_HCE, 0U);
	if (ret)
		return ret;
	ret = ufshc_enable_controller(host);
	if (ret)
		return ret;
	ufshc_clear_interrupts(host);
	/* The reference flow enables only UIC completion and power-mode completion
	 * while the M-PHY is being brought up.  In particular, a transient PHY
	 * status during DME_LINK_STARTUP must not terminate that command. */
	ufshc_write(host, UFSHC_REG_INTERRUPT_ENABLE, UFSHC_INT_UIC_COMPLETE_MASK | UFSHC_INT_UIC_POWER_MODE);
	return 0;
}

static int ufshc_wait_operational(struct ufshc_host *host)
{
	return ufshc_wait_mask(host, UFSHC_REG_CONTROLLER_STATUS, UFSHC_HCS_READY, UFSHC_HCS_READY);
}

static int ufshc_wait_peer_link_startup(struct ufshc_host *host)
{
	uint64_t start = time_us();

	/* ULSS is a status bit, not a UIC command completion.  A peer-initiated
	 * boot is allowed to race the host's first LINK STARTUP request. */
	while (!(ufshc_read(host, UFSHC_REG_INTERRUPT_STATUS) & UFSHC_INT_UIC_LINK_STARTUP)) {
		if (time_us() - start >= 100000U)
			return UFS_ERR_TIMEOUT;
	}
	return 0;
}

static void ufshc_prepare_utrd(void)
{
	struct ufshc_request_desc *utrd = ufshc_utrd();
	uintptr_t ucd = (uintptr_t)ufshc_ucd();
	uint32_t response_offset = offsetof(struct ufshc_command_desc, response_upiu) >> 2;
	uint32_t prdt_offset = offsetof(struct ufshc_command_desc, prdt) >> 2;

	memset(utrd, 0, sizeof(*utrd));
	utrd->command_base_lo = (uint32_t)ucd;
	utrd->command_base_hi = (uint32_t)((uint64_t)ucd >> 32);
	utrd->response_length = UFSHC_UPIU_SIZE >> 2;
	utrd->response_offset = response_offset;
	utrd->prdt_offset = prdt_offset;
	utrd->header[2] = UFSHC_OCS_MASK;
	data_sync_barrier();
}

static void ufshc_configure_slot(struct ufshc_host *host)
{
	uintptr_t utrd = (uintptr_t)ufshc_utrd();
	uintptr_t utmrd = (uintptr_t)ufshc_utmrd();

	ufshc_prepare_utrd();
	data_sync_barrier();

	ufshc_write(host, UFSHC_REG_UTRL_BASE_L, (uint32_t)utrd);
	ufshc_write(host, UFSHC_REG_UTRL_BASE_H, (uint32_t)((uint64_t)utrd >> 32));
	ufshc_write(host, UFSHC_REG_UTMRL_BASE_L, (uint32_t)utmrd);
	ufshc_write(host, UFSHC_REG_UTMRL_BASE_H, (uint32_t)((uint64_t)utmrd >> 32));
}

int ufshc_uic_command(struct ufshc_host *host, const struct ufshc_uic_cmd_args *args, uint32_t *result)
{
	uint64_t start;
	uint32_t status;
	uint32_t command_result;
	/* UFSHCI reports both ordinary UIC completion and power-mode completion
	 * through this interrupt group.  Stale events are cleared immediately
	 * before issuing the command below. */
	uint32_t completion_mask = UFSHC_INT_UIC_COMPLETE_MASK | UFSHC_INT_UIC_POWER_MODE;

	if (!host || !host->initialized || !args)
		return UFS_ERR_INVALID;
	/* UIC_COMMAND_COMPL is the ordinary completion indication.  UIC_POWER_MODE
	 * is also a valid completion event for power-control UIC commands, matching
	 * the native UFSHCI wait mask. */

	/* UIC commands are only accepted after the controller advertises the
	 * UIC_COMMAND_READY state.  This is especially important immediately
	 * after HCE, before the first LINK STARTUP command. */
	if (ufshc_wait_mask(host, UFSHC_REG_CONTROLLER_STATUS, 1U << 3, 1U << 3))
		return ufshc_fail(host, "UIC command ready", UFS_ERR_TIMEOUT);

	/* A stale completion bit would make a new command appear complete. */
	ufshc_clear_interrupts(host);
	ufshc_write(host, UFSHC_REG_UIC_ARG1, args->argument1);
	ufshc_write(host, UFSHC_REG_UIC_ARG2, args->argument2);
	ufshc_write(host, UFSHC_REG_UIC_ARG3, args->argument3);
	ufshc_write(host, UFSHC_REG_UIC_COMMAND, args->command);

	start = time_us();
	for (;;) {
		uint32_t phy_error;
		uint32_t dl_error;
		uint32_t nl_error;
		uint32_t tl_error;
		uint32_t dme_error;

		status = ufshc_read(host, UFSHC_REG_INTERRUPT_STATUS);
		if (status & UFSHC_INT_ERROR) {
			phy_error = ufshc_read(host, UFSHC_REG_UIC_ERROR_PHY_ADAPTER);
			dl_error = ufshc_read(host, UFSHC_REG_UIC_ERROR_DATA_LINK);
			nl_error = ufshc_read(host, UFSHC_REG_UIC_ERROR_NETWORK);
			tl_error = ufshc_read(host, UFSHC_REG_UIC_ERROR_TRANSPORT);
			dme_error = ufshc_read(host, UFSHC_REG_UIC_ERROR_DME);
			ufshc_write(host, UFSHC_REG_INTERRUPT_STATUS, status);
			if (args->command == UFSHC_UIC_LINK_STARTUP) {
				ufs_debug(
					"UFSHCI: link startup PHY status is=%08x err=%08x/%08x/%08x/%08x/%08x; waiting for completion\n",
					status, phy_error, dl_error, nl_error, tl_error, dme_error);
				if (status & completion_mask)
					break;
				continue;
			}
			printk_error("UFSHCI: UIC command 0x%02x failed\n", args->command);
			ufs_debug("UFSHCI: UIC error is=%08x err=%08x/%08x/%08x/%08x/%08x\n",
				status, phy_error, dl_error, nl_error, tl_error, dme_error);
			return UFS_ERR_IO;
		}
		if (status & completion_mask)
			break;
		if (time_us() - start >= ufshc_timeout(host))
			return ufshc_fail(host, "UIC command timeout", UFS_ERR_TIMEOUT);
	}
	ufshc_write(host, UFSHC_REG_INTERRUPT_STATUS, status & (completion_mask | UFSHC_INT_ERROR));
	if (result)
		*result = ufshc_read(host, UFSHC_REG_UIC_ARG2) & UFSHC_UIC_RESULT_MASK;
	command_result = ufshc_read(host, UFSHC_REG_UIC_ARG2) & UFSHC_UIC_RESULT_MASK;
	if (command_result) {
		printk_error("UFSHCI: UIC command 0x%02x returned 0x%02x\n", args->command, command_result);
		ufshc_log_state(host, "UIC result error");
		return UFS_ERR_IO;
	}
	return 0;
}

static uint32_t ufshc_uic_attribute(uint32_t attribute, uint16_t selector)
{
	/* UIC_ARG_MIB_SEL(attr, sel): attr in bits 31:16, selector in 15:0. */
	return ((attribute & 0xffffU) << 16) | selector;
}

int ufshc_dme_get_sel(struct ufshc_host *host, uint32_t attribute, uint16_t selector, uint32_t *value, bool peer)
{
	struct ufshc_uic_cmd_args args = {
		.command = peer ? UFSHC_UIC_DME_PEER_GET : UFSHC_UIC_DME_GET,
		.argument1 = ufshc_uic_attribute(attribute, selector),
	};
	uint32_t result = 0;
	int ret = UFS_ERR_IO;
	/* Native UFS_UIC_COMMAND_RETRIES=3 means three total attempts for peer
	 * attributes; local attributes are issued once. */
	unsigned int retries = peer ? 3U : 1U;

	while (retries--) {
		ret = ufshc_uic_command(host, &args, &result);
		if (!ret)
			break;
	}
	if (!ret && value)
		*value = ufshc_read(host, UFSHC_REG_UIC_ARG3);
	return ret;
}

int ufshc_dme_set_sel(struct ufshc_host *host, uint32_t attribute, uint16_t selector, uint32_t value, bool peer)
{
	struct ufshc_uic_cmd_args args = {
		.command = peer ? UFSHC_UIC_DME_PEER_SET : UFSHC_UIC_DME_SET,
		.argument1 = ufshc_uic_attribute(attribute, selector),
		.argument3 = value,
	};
	int ret = UFS_ERR_IO;
	unsigned int retries = peer ? 3U : 1U;

	while (retries--) {
		ret = ufshc_uic_command(host, &args, NULL);
		if (!ret)
			break;
	}
	return ret;
}

int ufshc_dme_get(struct ufshc_host *host, uint32_t attribute, uint32_t *value, bool peer)
{
	return ufshc_dme_get_sel(host, attribute, 0, value, peer);
}

int ufshc_dme_set(struct ufshc_host *host, uint32_t attribute, uint32_t value, bool peer)
{
	return ufshc_dme_set_sel(host, attribute, 0, value, peer);
}

int ufshc_get_max_power_mode(struct ufshc_host *host, struct ufshc_power_mode *mode)
{
	uint32_t value;
	int ret;

	if (!host || !mode)
		return UFS_ERR_INVALID;
	memset(mode, 0, sizeof(*mode));
	mode->pwr_rx = UFSHC_PWR_FAST;
	mode->pwr_tx = UFSHC_PWR_FAST;
	mode->hs_rate = UFSHC_HS_RATE_B;

	ret = ufshc_dme_get(host, UFSHC_PA_CONNECTEDRXDATALANES, &value, false);
	if (ret) {
			ufs_debug("UFSHCI: read connected RX lanes failed ret=%d\n", ret);
		return ret;
	}
	mode->lane_rx = (uint8_t)value;
	ret = ufshc_dme_get(host, UFSHC_PA_CONNECTEDTXDATALANES, &value, false);
	if (ret) {
			ufs_debug("UFSHCI: read connected TX lanes failed ret=%d\n", ret);
		return ret;
	}
	mode->lane_tx = (uint8_t)value;
	if (!mode->lane_rx || !mode->lane_tx)
		return UFS_ERR_IO;

	ret = ufshc_dme_get(host, UFSHC_PA_MAXRXHSGEAR, &value, false);
	if (ret) {
			ufs_debug("UFSHCI: read local max HS gear failed ret=%d\n", ret);
		return ret;
	}
	mode->gear_rx = (uint8_t)value;
	if (!mode->gear_rx) {
		ret = ufshc_dme_get(host, UFSHC_PA_MAXRXPWMGEAR, &value, false);
		if (ret) {
				ufs_debug("UFSHCI: read local max PWM gear failed ret=%d\n", ret);
			return ret;
		}
		mode->gear_rx = (uint8_t)value;
		mode->pwr_rx = UFSHC_PWR_SLOW;
	}

	ret = ufshc_dme_get(host, UFSHC_PA_MAXRXHSGEAR, &value, true);
	if (ret) {
			ufs_debug("UFSHCI: read peer max HS gear failed ret=%d\n", ret);
		return ret;
	}
	mode->gear_tx = (uint8_t)value;
	if (!mode->gear_tx) {
		ret = ufshc_dme_get(host, UFSHC_PA_MAXRXPWMGEAR, &value, true);
		if (ret) {
				ufs_debug("UFSHCI: read peer max PWM gear failed ret=%d\n", ret);
			return ret;
		}
		mode->gear_tx = (uint8_t)value;
		mode->pwr_tx = UFSHC_PWR_SLOW;
	}
	if (!mode->gear_rx || !mode->gear_tx)
		return UFS_ERR_IO;
	ufs_debug("UFSHCI: max mode pwr=%u/%u gear=%u/%u lane=%u/%u\n", mode->pwr_tx, mode->pwr_rx, mode->gear_tx,
		mode->gear_rx, mode->lane_tx, mode->lane_rx);
	return 0;
}

int ufshc_change_power_mode(struct ufshc_host *host, const struct ufshc_power_mode *mode)
{
	struct ufshc_uic_cmd_args args;
	uint32_t status;
	uint64_t start;
	int ret;

	if (!host || !mode || !mode->gear_rx || !mode->gear_tx || !mode->lane_rx || !mode->lane_tx)
		return UFS_ERR_INVALID;
	ufs_debug("UFSHCI: change power mode pwr=%u/%u gear=%u/%u lane=%u/%u hs_rate=%u\n", mode->pwr_tx,
		mode->pwr_rx, mode->gear_tx, mode->gear_rx, mode->lane_tx, mode->lane_rx, mode->hs_rate);
	/* Match ufshcd_dme_configure_adapt(): initial adaptation is only valid
	 * for HS Gear 4; PWM and lower gears must explicitly select no
	 * adaptation. */
	ret = ufshc_dme_set(host, UFSHC_PA_TXHSADAPTTYPE,
		mode->pwr_tx == UFSHC_PWR_FAST && mode->gear_tx >= UFSHC_PA_INITIAL_ADAPT_GEAR ?
			UFSHC_PA_INITIAL_ADAPT :
			UFSHC_PA_NO_ADAPT,
		false);
	if (ret) {
		ufs_debug("UFSHCI: set TX adaptation failed ret=%d\n", ret);
		return ret;
	}
	ret = ufshc_dme_set(host, UFSHC_PA_RXGEAR, mode->gear_rx, false);
	if (ret) {
		ufs_debug("UFSHCI: set RX gear failed ret=%d\n", ret);
		return ret;
	}
	ret = ufshc_dme_set(host, UFSHC_PA_ACTIVERXDATALANES, mode->lane_rx, false);
	if (ret) {
		ufs_debug("UFSHCI: set active RX lanes failed ret=%d\n", ret);
		return ret;
	}
	ret = ufshc_dme_set(host, UFSHC_PA_RXTERMINATION, mode->pwr_rx == UFSHC_PWR_FAST, false);
	if (ret) {
		ufs_debug("UFSHCI: set RX termination failed ret=%d\n", ret);
		return ret;
	}
	ret = ufshc_dme_set(host, UFSHC_PA_TXGEAR, mode->gear_tx, false);
	if (ret) {
		ufs_debug("UFSHCI: set TX gear failed ret=%d\n", ret);
		return ret;
	}
	ret = ufshc_dme_set(host, UFSHC_PA_ACTIVETXDATALANES, mode->lane_tx, false);
	if (ret) {
		ufs_debug("UFSHCI: set active TX lanes failed ret=%d\n", ret);
		return ret;
	}
	ret = ufshc_dme_set(host, UFSHC_PA_TXTERMINATION, mode->pwr_tx == UFSHC_PWR_FAST, false);
	if (ret) {
		ufs_debug("UFSHCI: set TX termination failed ret=%d\n", ret);
		return ret;
	}
	if (mode->pwr_rx == UFSHC_PWR_FAST || mode->pwr_tx == UFSHC_PWR_FAST) {
		ret = ufshc_dme_set(host, UFSHC_PA_HSSERIES, mode->hs_rate, false);
		if (ret) {
			ufs_debug("UFSHCI: set HS series failed ret=%d\n", ret);
			return ret;
		}
	}
	args = (struct ufshc_uic_cmd_args){
		.command = UFSHC_UIC_DME_SET,
		.argument1 = ufshc_uic_attribute(UFSHC_PA_PWRMODE, 0),
		.argument3 = ((uint32_t)mode->pwr_rx << 4) | mode->pwr_tx,
	};
	ret = ufshc_uic_command(host, &args, &status);
	if (ret) {
		printk_error("UFSHCI: request power mode change failed ret=%d\n", ret);
		return ret;
	}
	start = time_us();
	for (;;) {
		uint32_t upmcrs = (ufshc_read(host, UFSHC_REG_CONTROLLER_STATUS) & UFSHC_HCS_UPMCRS_MASK) >>
				  UFSHC_HCS_UPMCRS_SHIFT;

		if (upmcrs == UFSHC_PWR_LOCAL)
			return 0;
		if (time_us() - start >= ufshc_timeout(host))
			return ufshc_fail(host, "power mode transition timeout",
				upmcrs == UFSHC_PWR_OK ? UFS_ERR_TIMEOUT : UFS_ERR_IO);
	}
}

int ufshc_init(struct ufshc_host *host, const struct ufshc_config *config)
{
	int ret;

	if (!host || !config || !config->base) {
		printk_error("UFSHCI: invalid host configuration\n");
		return UFS_ERR_INVALID;
	}
	printk_info("UFSHCI: init base=%p timeout_us=%u\n", (void *)config->base,
		config->timeout_us ? config->timeout_us : UFSHC_TIMEOUT_US);
	memset(host, 0, sizeof(*host));
	memset(ufshc_dma_pool, 0, sizeof(ufshc_dma_pool));
	ufshc_devman_ocs_reported = false;
	printk_info("UFSHCI: descriptor SRAM utrd=%p ucd=%p utmrd=%p\n", (void *)ufshc_utrd(), (void *)ufshc_ucd(),
		(void *)ufshc_utmrd());
	host->base = config->base;
	host->timeout_us = config->timeout_us ? config->timeout_us : UFSHC_TIMEOUT_US;
	if (config->platform) {
		host->platform_ops = *config->platform;
		if (config->platform_priv)
			host->platform_ops.priv = config->platform_priv;
		host->platform = &host->platform_ops;
	}

	if (host->platform && host->platform->enable) {
		host->platform_active = true;
		ret = host->platform->enable(host->platform->priv);
		if (ret) {
			printk_error("UFSHCI: platform enable failed ret=%d\n", ret);
			goto disable_platform;
		}
	}
	if (host->platform && host->platform->phy_init) {
		host->platform_active = true;
		ret = host->platform->phy_init(host->platform->priv);
		if (ret) {
			printk_error("UFSHCI: PHY init failed ret=%d\n", ret);
			goto disable_platform;
		}
	}
	if (host->platform && host->platform->prepare) {
		host->platform_active = true;
		ret = host->platform->prepare(host->base, host->platform->priv);
		if (ret) {
			printk_error("UFSHCI: controller prepare failed ret=%d\n", ret);
			goto disable_platform;
		}
	}

	host->capabilities = ufshc_read(host, UFSHC_REG_CAP);
	host->version = ufshc_read(host, UFSHC_REG_VERSION);
	printk_info("UFSHCI: CAP=0x%08x VERSION=0x%08x\n", host->capabilities, host->version);
	ufshc_clear_interrupts(host);
	/* Keep all interrupt sources masked across HCE initialization.  The
	 * native flow enables only UIC completion/error reporting before link
	 * startup and enables transfer/task completion after the link is up. */
	ufshc_write(host, UFSHC_REG_INTERRUPT_ENABLE, 0U);
	if (host->platform && host->platform->device_reset)
		host->platform->device_reset(host->base, host->platform->priv);
	/* A warm re-entry may leave HCE asserted.  Native UFSHCI startup first
	 * drives the controller through its disabled state so the next HCE write
	 * performs the required 1->0->1 initialization transition. */
	if (ufshc_read(host, UFSHC_REG_CONTROLLER_ENABLE) & UFSHC_HCE) {
		ufshc_write(host, UFSHC_REG_CONTROLLER_ENABLE, 0U);
		ret = ufshc_wait_mask(host, UFSHC_REG_CONTROLLER_ENABLE, UFSHC_HCE, 0U);
		if (ret)
			goto disable_platform;
	}
	host->controller_enabled = true;
	ret = ufshc_enable_controller(host);
	if (ret) {
		printk_error("UFSHCI: HCE enable failed ret=%d\n", ret);
		goto disable_platform;
	}
	ufshc_write(host, UFSHC_REG_INTERRUPT_ENABLE, UFSHC_INT_UIC_COMPLETE_MASK | UFSHC_INT_UIC_POWER_MODE);
	/* Allow the UIC helper to be used during the link phase as well. */
	host->initialized = true;
	{
		bool skip_phy_setup = false;

		/* The native DME_LINKSTARTUP_RETRIES value counts retries after the
		 * initial attempt, so the loop has four total iterations. */
		for (unsigned int retry = 0; retry <= UFSHC_LINK_STARTUP_RETRIES; ++retry) {
			bool retry_without_phy = skip_phy_setup;

			/* A peer-initiated boot skips PHY programming for exactly one
			 * subsequent LINK STARTUP attempt, matching the native skip_no
			 * state machine. */
			skip_phy_setup = false;
			if (!retry_without_phy && host->platform && host->platform->link_startup) {
				ret = host->platform->link_startup(host, host->platform->priv);
				if (ret) {
					printk_error("UFSHCI: platform PHY link setup failed attempt=%u ret=%d\n",
						retry + 1U, ret);
					if (retry < UFSHC_LINK_STARTUP_RETRIES) {
						ret = ufshc_reinitialize_controller(host);
						if (ret)
							break;
					}
					continue;
				}
			}
			struct ufshc_uic_cmd_args args = {
				.command = UFSHC_UIC_LINK_STARTUP,
			};

			ret = ufshc_uic_command(host, &args, NULL);
			if (!ret) {
				uint32_t controller_status = ufshc_read(host, UFSHC_REG_CONTROLLER_STATUS);

				if (controller_status & UFSHC_HCS_DEVICE_PRESENT)
					break;
				printk_warning("UFSHCI: link is up but device is not present\n");

				/* Match SUPPORT_PEER_INITED_BOOT from the reference driver.  The
				 * peer's ULSS event is cleared and the host retries LINK STARTUP
				 * without reprogramming the PHY. */
				if (!ufshc_wait_peer_link_startup(host)) {
					ufshc_write(host, UFSHC_REG_INTERRUPT_STATUS, UFSHC_INT_UIC_LINK_STARTUP);
					if (retry < UFSHC_LINK_STARTUP_RETRIES - 1U) {
						skip_phy_setup = true;
						continue;
					}
					/* On the last retry before the final attempt, the native
					 * peer-initiated-boot path resets RST_n and reinitializes HCE,
					 * then performs one final startup with PHY setup enabled. */
					if (retry == UFSHC_LINK_STARTUP_RETRIES - 1U && host->platform &&
						host->platform->device_reset)
						host->platform->device_reset(host->base, host->platform->priv);
				} else if (retry + 1U == 3U) {
					/* No peer ULSS event arrived.  Match the native final
					 * retry by resetting RST_n before the last startup attempt. */
					if (host->platform && host->platform->device_reset)
						host->platform->device_reset(host->base, host->platform->priv);
				}
				ret = UFS_ERR_IO;
			}

			if (retry < UFSHC_LINK_STARTUP_RETRIES) {
				ret = ufshc_reinitialize_controller(host);
				if (ret)
					break;
			}
		}
	}

	if (ret)
		goto disable_platform;

	if (!(ufshc_read(host, UFSHC_REG_CONTROLLER_STATUS) & UFSHC_HCS_DEVICE_PRESENT)) {
		ret = UFS_ERR_IO;
		printk_error("UFSHCI: device absent after link startup\n");
		goto disable_platform;
	}

	if (host->platform && host->platform->link_up) {
		ret = host->platform->link_up(host, host->platform->priv);
		if (ret) {
			printk_error("UFSHCI: platform link-up configuration failed ret=%d\n", ret);
			goto disable_platform;
		}
	}
	/* Early-boot polling expects one completion interrupt per request. */
	ufshc_write(host, UFSHC_REG_UTRL_INT_AGG_CONTROL, 0U);
	ufshc_write(host, UFSHC_REG_INTERRUPT_ENABLE,
		UFSHC_INT_TRANSFER_COMPLETE | UFSHC_INT_TASK_COMPLETE | UFSHC_INT_UIC_COMPLETE_MASK |
			UFSHC_INT_UIC_POWER_MODE | UFSHC_INT_ERROR);
	ufshc_configure_slot(host);
	ret = ufshc_wait_operational(host);
	if (ret) {
		printk_error("UFSHCI: controller not operational ret=%d\n", ret);
		goto disable_platform;
	}

	ufshc_write(host, UFSHC_REG_UTMRL_RUN_STOP, 1U);
	ufshc_write(host, UFSHC_REG_UTRL_RUN_STOP, 1U);
	printk_info("UFSHCI: init complete\n");
	return 0;

disable_platform:
	printk_error("UFSHCI: init aborted ret=%d\n", ret);
	ufshc_log_state(host, "init abort");
	if (host->controller_enabled) {
		ufshc_write(host, UFSHC_REG_UTMRL_RUN_STOP, 0U);
		ufshc_write(host, UFSHC_REG_UTRL_RUN_STOP, 0U);
		ufshc_write(host, UFSHC_REG_CONTROLLER_ENABLE, 0U);
		host->controller_enabled = false;
	}

	if (host->platform_active && host->platform && host->platform->disable)
		host->platform->disable(host->platform->priv);

	host->platform_active = false;
	host->initialized = false;
	return ret;
}

static int ufshc_exec_devman(struct ufshc_host *host)
{
	struct ufshc_request_desc *utrd = ufshc_utrd();
	struct ufshc_command_desc *ucd = ufshc_ucd();
	uint32_t status;
	uint32_t timeout_us = ufshc_timeout(host);
	uint64_t start;
	int ret = 0;

	/* The reference Sunxi driver gives NOP and Query UTP requests a 3 s
	 * completion window, independently of the shorter UIC/transfer timeout. */
	if (timeout_us < UFSHC_DEV_MANAGEMENT_TIMEOUT_US)
		timeout_us = UFSHC_DEV_MANAGEMENT_TIMEOUT_US;

	ufshc_prepare_utrd();
	utrd->header[0] = UFSHC_REQ_INT | ufshc_cmd_type(host, UFSHC_REQ_CMD_TYPE_DEV_MGMT);
	ufshc_sync_for_device(host, NULL, 0);
	ufshc_clear_interrupts(host);
	ufshc_write(host, UFSHC_REG_UTRL_DOOR_BELL, 1U);

	start = time_us();

	for (;;) {
		status = ufshc_read(host, UFSHC_REG_INTERRUPT_STATUS);
		if (status & UFSHC_INT_ERROR) {
			ret = UFS_ERR_IO;
			break;
		}
		if (status & UFSHC_INT_TRANSFER_COMPLETE)
			break;
		if (time_us() - start >= timeout_us) {
			ret = UFS_ERR_TIMEOUT;
			break;
		}
	}

	if (status)
		ufshc_write(host, UFSHC_REG_INTERRUPT_STATUS, status);
	if (ret) {
		printk_error("UFSHCI: device-management request failed ret=%d status=0x%08x\n", ret, status);
		ufshc_log_state(host, "device-management error");
		ufshc_abort_transfer(host);
		ufshc_sync_for_cpu(host, NULL, 0);
		return ret;
	}
	ufshc_sync_for_cpu(host, NULL, 0);

	if (utrd->header[2] & UFSHC_OCS_MASK) {
		if (!ufshc_devman_ocs_reported) {
			printk_error("UFSHCI: device-management OCS=0x%x\n", utrd->header[2] & UFSHC_OCS_MASK);
			ufs_debug(
				"UFSHCI: devman utrd=%p ucd=%p hdr=%08x/%08x/%08x/%08x rsp=%u@%u prdt=%u@%u req=%08x\n",
				(void *)utrd, (void *)ucd, utrd->header[0],
				utrd->header[1], utrd->header[2], utrd->header[3], utrd->response_length,
				utrd->response_offset, utrd->prdt_length, utrd->prdt_offset,
				ufs_be32(ufs_load32(ucd->command_upiu)));
			ufshc_devman_ocs_reported = true;
		}
		return UFS_ERR_IO;
	}
	return 0;
}

int ufshc_nop(struct ufshc_host *host)
{
	struct ufshc_command_desc *ucd = ufshc_ucd();
	uint8_t *command;
	uint8_t *response;
	int ret;

	if (!host || !host->initialized)
		return UFS_ERR_INVALID;
	memset(ucd, 0, sizeof(*ucd));
	command = ucd->command_upiu;
	response = ucd->response_upiu;
	ufs_store32(&command[0], ufs_be32((UFSHC_UPIU_NOP_OUT << 24) | 0x1fU));
	ufs_store32(&command[4], 0U);
	ufs_store32(&command[8], 0U);
	ret = ufshc_exec_devman(host);
	if (ret)
		return ret;
	if (response[0] != UFSHC_UPIU_NOP_IN) {
		printk_error("UFSHCI: invalid NOP response type=0x%02x\n", response[0]);
		return UFS_ERR_IO;
	}
	return 0;
}

int ufshc_query_flag_op(struct ufshc_host *host, uint8_t idn, uint8_t opcode, bool *value)
{
	struct ufshc_command_desc *ucd = ufshc_ucd();
	uint8_t *command;
	uint8_t *response;
	uint32_t result;
	uint8_t query_func;
	int ret;

	if (!host || !host->initialized)
		return UFS_ERR_INVALID;
	if (opcode == UFSHC_QUERY_OPCODE_READ_FLAG) {
		if (!value)
			return UFS_ERR_INVALID;
		query_func = UFSHC_QUERY_FUNC_STANDARD_READ;
	} else if (opcode == UFSHC_QUERY_OPCODE_SET_FLAG || opcode == UFSHC_QUERY_OPCODE_CLEAR_FLAG ||
		   opcode == UFSHC_QUERY_OPCODE_TOGGLE_FLAG) {
		query_func = UFSHC_QUERY_FUNC_STANDARD_WRITE;
	} else {
		return UFS_ERR_INVALID;
	}
	memset(ucd, 0, sizeof(*ucd));
	command = ucd->command_upiu;
	response = ucd->response_upiu;
	ufs_store32(&command[0], ufs_be32(UFSHC_UPIU_QUERY_REQ << 24));
	ufs_store32(&command[4], ufs_be32((uint32_t)query_func << 16));
	ufs_store32(&command[8], 0U);
	command[12] = opcode;
	command[13] = idn;
	ret = ufshc_exec_devman(host);
	if (ret)
		return ret;
	if (response[0] != UFSHC_UPIU_QUERY_RSP) {
		printk_error("UFSHCI: invalid Query Flag response type=0x%02x idn=0x%02x\n", response[0], idn);
		return UFS_ERR_IO;
	}
	result = ufs_be32(ufs_load32(&response[4])) & 0xffffU;
	if ((result >> 8) != 0U) {
		printk_error("UFSHCI: Query Flag failed idn=0x%02x opcode=0x%02x result=0x%04x\n", idn, opcode, result);
		return UFS_ERR_IO;
	}
	if (opcode == UFSHC_QUERY_OPCODE_READ_FLAG)
		*value = (ufs_be32(ufs_load32(&response[20])) & 0xffU) != 0U;
	return 0;
}

int ufshc_query_flag(struct ufshc_host *host, uint8_t idn, bool set, bool *value)
{
	return ufshc_query_flag_op(host, idn, set ? UFSHC_QUERY_OPCODE_SET_FLAG : UFSHC_QUERY_OPCODE_READ_FLAG, value);
}

int ufshc_query_attribute(
	struct ufshc_host *host, uint8_t idn, uint8_t index, uint8_t selector, uint32_t *value, bool write)
{
	struct ufshc_command_desc *ucd = ufshc_ucd();
	uint8_t *command;
	uint8_t *response;
	uint32_t result;
	int ret;

	if (!host || !host->initialized || !value)
		return UFS_ERR_INVALID;
	memset(ucd, 0, sizeof(*ucd));
	command = ucd->command_upiu;
	response = ucd->response_upiu;
	ufs_store32(&command[0], ufs_be32(UFSHC_UPIU_QUERY_REQ << 24));
	ufs_store32(&command[4],
		ufs_be32((write ? UFSHC_QUERY_FUNC_STANDARD_WRITE : UFSHC_QUERY_FUNC_STANDARD_READ) << 16));
	ufs_store32(&command[8], 0U);
	command[12] = write ? UFSHC_QUERY_OPCODE_WRITE_ATTR : UFSHC_QUERY_OPCODE_READ_ATTR;
	command[13] = idn;
	command[14] = index;
	command[15] = selector;
	if (write) {
		ufs_store32(&command[20], ufs_be32(*value));
	}
	ret = ufshc_exec_devman(host);
	if (ret)
		return ret;
	if (response[0] != UFSHC_UPIU_QUERY_RSP) {
		printk_error("UFSHCI: invalid Query Attribute response type=0x%02x idn=0x%02x\n", response[0], idn);
		return UFS_ERR_IO;
	}
	result = ufs_be32(ufs_load32(&response[4])) & 0xffffU;
	if ((result >> 8) != 0U) {
		printk_error("UFSHCI: Query Attribute failed idn=0x%02x result=0x%04x\n", idn, result);
		return UFS_ERR_IO;
	}
	if (!write)
		*value = ufs_be32(ufs_load32(&response[20]));
	return 0;
}

int ufshc_query_descriptor_op(struct ufshc_host *host, uint8_t opcode, uint8_t idn, uint8_t index, uint8_t selector,
	void *buffer, size_t buffer_len, size_t *actual_len)
{
	struct ufshc_command_desc *ucd = ufshc_ucd();
	uint8_t *command;
	uint8_t *response;
	uint32_t response_result;
	uint32_t response_length;
	uint32_t descriptor_length;
	int ret;

	if (!host || !host->initialized || !buffer || buffer_len < 2U || buffer_len > 255U ||
		(opcode != UFSHC_QUERY_OPCODE_READ_DESC && opcode != UFSHC_QUERY_OPCODE_WRITE_DESC))
		return UFS_ERR_INVALID;
	if (actual_len)
		*actual_len = 0;
	memset(ucd, 0, sizeof(*ucd));
	command = ucd->command_upiu;
	response = ucd->response_upiu;
	ufs_store32(&command[0], ufs_be32(UFSHC_UPIU_QUERY_REQ << 24));
	ufs_store32(&command[4], ufs_be32((opcode == UFSHC_QUERY_OPCODE_WRITE_DESC ? UFSHC_QUERY_FUNC_STANDARD_WRITE :
										     UFSHC_QUERY_FUNC_STANDARD_READ)
					  << 16));
	ufs_store32(&command[8], opcode == UFSHC_QUERY_OPCODE_WRITE_DESC ? ufs_be32((uint32_t)buffer_len) : 0U);
	command[12] = opcode;
	command[13] = idn;
	command[14] = index;
	command[15] = selector;
	command[18] = (uint8_t)(buffer_len >> 8);
	command[19] = (uint8_t)buffer_len;
	if (opcode == UFSHC_QUERY_OPCODE_WRITE_DESC)
		memcpy(&command[UFSHC_UPIU_DATA_OFFSET], buffer, buffer_len);
	ret = ufshc_exec_devman(host);
	if (ret)
		return ret;
	if (response[0] != UFSHC_UPIU_QUERY_RSP)
		return UFS_ERR_IO;
	response_result = ufs_be32(ufs_load32(&response[4])) & 0xffffU;
	if ((response_result >> 8) != 0U)
		return UFS_ERR_IO;
	/* The response descriptor is carried in the UPIU data segment.  Its
	 * actual length is reported by the response header's data-segment-length
	 * field (DW2, bytes 10..11); the Query OSF length is only the requested
	 * transfer size and may be echoed unchanged by the device. */
	response_length = ((uint32_t)response[10] << 8) | response[11];
	descriptor_length = ((uint32_t)response[18] << 8) | response[19];
	if (opcode == UFSHC_QUERY_OPCODE_WRITE_DESC) {
		return 0;
	}
	if (response_length < 2U || response_length > buffer_len ||
		response_length > UFSHC_UPIU_SIZE - UFSHC_UPIU_DATA_OFFSET)
		return UFS_ERR_IO;
	/* A short read of the descriptor header is valid; only bytes reported in
	 * the response data segment are copied into the caller's buffer. */
	memcpy(buffer, &response[UFSHC_UPIU_DATA_OFFSET], response_length);
	if (actual_len)
		/* The Query OSF length is the descriptor length reported by the
		 * device, while the UPIU data-segment length describes the bytes that
		 * were physically returned.  Never report bytes that were not copied. */
		*actual_len = descriptor_length >= 2U && descriptor_length < response_length ? descriptor_length :
											       response_length;
	return 0;
}

int ufshc_query_descriptor(struct ufshc_host *host, uint8_t idn, uint8_t index, uint8_t selector, void *buffer,
	size_t buffer_len, size_t *actual_len)
{
	return ufshc_query_descriptor_op(
		host, UFSHC_QUERY_OPCODE_READ_DESC, idn, index, selector, buffer, buffer_len, actual_len);
}

int ufshc_task_request(
	struct ufshc_host *host, uint8_t lun, uint8_t function, uint16_t task_id, uint8_t *service_response)
{
	struct ufshc_task_request_desc *utmrd = ufshc_utmrd();
	uint32_t status = 0;
	uint32_t response_header;
	uint64_t start;
	int ret = 0;

	if (!host || !host->initialized || !function)
		return UFS_ERR_INVALID;
	if (ufshc_read(host, UFSHC_REG_UTMRL_DOOR_BELL) & 1U)
		return UFS_ERR_IO;

	memset(utmrd, 0, sizeof(*utmrd));
	utmrd->header[0] = UFSHC_REQ_INT | ufshc_cmd_type(host, UFSHC_REQ_CMD_TYPE_DEV_MGMT);
	utmrd->header[2] = UFSHC_OCS_MASK;
	utmrd->request_header[0] = ufs_be32((UFSHC_UPIU_TASK_REQ << 24) | ((uint32_t)lun << 8));
	utmrd->request_header[1] = ufs_be32((uint32_t)function << 16);
	utmrd->request_header[2] = 0U;
	/* The LUN is carried by the task-request UPIU header.  Input Parameter 1
	 * is the task tag for ABORT/QUERY operations; the remaining parameters
	 * are reserved by the UFS task-management protocol. */
	utmrd->input_param[0] = ufs_be32(task_id);
	utmrd->input_param[1] = 0U;
	utmrd->input_param[2] = 0U;
	ufshc_sync_for_device(host, NULL, 0);
	ufshc_clear_interrupts(host);
	ufshc_write(host, UFSHC_REG_UTMRL_DOOR_BELL, 1U);
	start = time_us();
	for (;;) {
		status = ufshc_read(host, UFSHC_REG_INTERRUPT_STATUS);
		if (status & UFSHC_INT_ERROR) {
			ret = UFS_ERR_IO;
			break;
		}
		if (status & UFSHC_INT_TASK_COMPLETE)
			break;
		if (time_us() - start >= ufshc_timeout(host)) {
			ret = UFS_ERR_TIMEOUT;
			break;
		}
	}
	if (status)
		ufshc_write(host, UFSHC_REG_INTERRUPT_STATUS, status);
	if (ret) {
		/* UTMRLCLR follows the same write-one-to-clear slot encoding. */
		ufshc_write(host, UFSHC_REG_UTMRL_LIST_CLEAR, 1U);
		ufshc_wait_mask(host, UFSHC_REG_UTMRL_DOOR_BELL, 1U, 0U);
		return ret;
	}
	ufshc_sync_for_cpu(host, NULL, 0);
	if (utmrd->header[2] & UFSHC_OCS_MASK)
		return UFS_ERR_IO;
	response_header = ufs_be32(ufs_load32(&utmrd->response_header[0]));
	if ((response_header >> 24) != UFSHC_UPIU_TASK_RSP)
		return UFS_ERR_IO;
	if (service_response)
		*service_response = (uint8_t)(ufs_be32(ufs_load32(&utmrd->output_param[0])) & 0xffU);
	return 0;
}

int ufshc_exec(struct ufshc_host *host, struct ufshc_request *request)
{
	struct ufshc_request_desc *utrd = ufshc_utrd();
	struct ufshc_command_desc *ucd = ufshc_ucd();
	uint8_t *command;
	uint8_t *response;
	uintptr_t buffer;
	size_t remaining;
	unsigned int entries = 0;
	uint32_t flags;
	uint32_t status;
	uint64_t start;
	int transfer_ret = 0;

	if (!host || !request || !host->initialized || !request->cdb_len || request->cdb_len > UFSHC_CDB_SIZE)
		return UFS_ERR_INVALID;
	if (request->data_len && !request->data)
		return UFS_ERR_INVALID;
	if (request->data_len && ((uintptr_t)request->data & 3U))
		return UFS_ERR_INVALID;
	if (request->data_len > (size_t)UFSHC_MAX_PRDT * UFSHC_PRDT_MAX_BYTES)
		return UFS_ERR_INVALID;
	if (request->data_len > UINT32_MAX)
		return UFS_ERR_INVALID;
	if (request->data_len && (request->data_len & 3U))
		return UFS_ERR_INVALID;
	request->response_type = 0;
	request->task_response = 0;
	request->status = 0;
	request->sense_length = 0;
	request->residual_transfer_count = 0;

	ufshc_prepare_utrd();
	memset(ucd, 0, sizeof(*ucd));
	command = ucd->command_upiu;
	response = ucd->response_upiu;
	flags = request->write ? UPIU_FLAG_WRITE : (request->data_len ? UPIU_FLAG_READ : 0U);
	ufs_store32(&command[0], ufs_be32((UFSHC_UPIU_COMMAND << 24) | (flags << 16) | ((uint32_t)request->lun << 8)));
	ufs_store32(&command[4], ufs_be32(0U)); /* SCSI command set, tag 0. */
	ufs_store32(&command[8], 0U);
	ufs_store32(&command[12], ufs_be32((uint32_t)request->data_len));
	memcpy(&command[16], request->cdb, request->cdb_len);

	buffer = (uintptr_t)request->data;
	remaining = request->data_len;
	while (remaining) {
		size_t length = remaining > UFSHC_PRDT_MAX_BYTES ? UFSHC_PRDT_MAX_BYTES : remaining;
		struct ufshc_prd *prd = &ucd->prdt[entries++];
		prd->base = (uint32_t)buffer;
		prd->upper = (uint32_t)((uint64_t)buffer >> 32);
		/* UFSHCI PRDT byte count is encoded as (bytes - 1) with the
		 * reserved low bits set, matching the native Sunxi driver. */
		prd->size = (uint32_t)(length - 1U) | 0x3U;
		buffer += length;
		remaining -= length;
	}
	utrd->prdt_length = (uint16_t)entries;
	utrd->header[0] =
		UFSHC_REQ_INT | ufshc_cmd_type(host, UFSHC_REQ_CMD_TYPE_SCSI) |
		(request->write ? UFSHC_REQ_HOST_TO_DEVICE : (request->data_len ? UFSHC_REQ_DEVICE_TO_HOST : 0U));
	utrd->header[1] = 0U;
	utrd->header[2] = UFSHC_OCS_MASK;
	utrd->header[3] = 0U;
	ufshc_sync_for_device(host, request->data, request->data_len);

	ufshc_clear_interrupts(host);
	ufshc_write(host, UFSHC_REG_UTRL_DOOR_BELL, 1U);
	start = time_us();
	for (;;) {
		status = ufshc_read(host, UFSHC_REG_INTERRUPT_STATUS);
		if (status & UFSHC_INT_ERROR) {
			transfer_ret = UFS_ERR_IO;
			break;
		}
		if (status & UFSHC_INT_TRANSFER_COMPLETE)
			break;
		if (time_us() - start >= ufshc_timeout(host)) {
			transfer_ret = UFS_ERR_TIMEOUT;
			break;
		}
	}
	if (status)
		ufshc_write(host, UFSHC_REG_INTERRUPT_STATUS, status);
	if (transfer_ret)
		ufshc_abort_transfer(host);
	/* A device-to-host transfer invalidates the destination.  For writes,
	 * retain the caller's cache lines after the controller has consumed them. */
	ufshc_sync_for_cpu(host, request->write ? NULL : request->data, request->write ? 0U : request->data_len);
	if (transfer_ret)
		return transfer_ret;

	if ((utrd->header[2] & UFSHC_OCS_MASK) != 0U)
		return UFS_ERR_IO;
	request->response_type = response[0];
	if (request->response_type != UFSHC_UPIU_RESPONSE && request->response_type != UFSHC_UPIU_REJECT)
		return UFS_ERR_IO;
	request->residual_transfer_count = ufs_be32(ufs_load32(&response[12]));
	/* For a SCSI response, DW1[7:0] is the SCSI status byte.  DW1[15:8]
	 * is the UPIU response code (task-management responses use a different
	 * output field); retain it in task_response for the existing API. */
	{
		uint32_t response_result = ufs_be32(ufs_load32(&response[4]));

		request->task_response = (uint8_t)((response_result >> 8) & 0xffU);
		request->status = (uint8_t)(response_result & 0xffU);
	}
	{
		uint32_t sense_length = ufs_be32(ufs_load32(&response[32])) >> 16;
		if (sense_length > UFSHC_SCSI_SENSE_SIZE)
			sense_length = UFSHC_SCSI_SENSE_SIZE;
		request->sense_length = (uint8_t)sense_length;
	}
	if (request->sense_length)
		memcpy(request->sense, &response[UFSHC_SCSI_SENSE_OFFSET], request->sense_length);
	if (request->response_type == UFSHC_UPIU_REJECT || request->task_response != 0U || request->status != 0U)
		return UFS_ERR_IO;
	return 0;
}

void ufshc_exit(struct ufshc_host *host)
{
	if (!host)
		return;
	if (host->controller_enabled) {
		ufshc_write(host, UFSHC_REG_UTMRL_RUN_STOP, 0U);
		ufshc_write(host, UFSHC_REG_UTRL_RUN_STOP, 0U);
		/* Give the controller a chance to quiesce outstanding list state
		 * before HCE is removed.  Early boot callers do not need to surface a
		 * shutdown timeout, but disabling in order avoids a stale doorbell on
		 * the next initialization. */
		ufshc_wait_mask(host, UFSHC_REG_UTMRL_RUN_STOP, 1U, 0U);
		ufshc_wait_mask(host, UFSHC_REG_UTRL_RUN_STOP, 1U, 0U);
		ufshc_write(host, UFSHC_REG_CONTROLLER_ENABLE, 0U);
		ufshc_wait_mask(host, UFSHC_REG_CONTROLLER_ENABLE, UFSHC_HCE, 0U);
		host->controller_enabled = false;
	}
	if (host->platform_active && host->platform && host->platform->disable)
		host->platform->disable(host->platform->priv);
	host->platform_active = false;
	host->initialized = false;
}
