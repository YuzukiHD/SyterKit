/* SPDX-License-Identifier: GPL-2.0+ */

/* SCSI command layer for a UFS logical unit. */
/* Requests are serialized by the one-slot host-controller transport. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <limits.h>
#include <log.h>
#include <timer.h>

#include <drivers/ufs/scsi.h>
#include <drivers/ufs/ufshc.h>

#define UFS_SCSI_ERR		       (-1)
#define UFS_SCSI_CAPACITY16_LENGTH     32U
#define UFS_SCSI_SENSE_TRANSFER_LENGTH 20U

static uint32_t scsi_be32(const uint8_t *data)
{
	return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | ((uint32_t)data[2] << 8) | data[3];
}

static uint64_t scsi_be64(const uint8_t *data)
{
	uint64_t value = 0;
	unsigned int i;

	for (i = 0; i < 8; i++)
		value = (value << 8) | data[i];
	return value;
}

static void scsi_set_be32(uint8_t *data, uint32_t value)
{
	data[0] = (uint8_t)(value >> 24);
	data[1] = (uint8_t)(value >> 16);
	data[2] = (uint8_t)(value >> 8);
	data[3] = (uint8_t)value;
}

static void scsi_set_be64(uint8_t *data, uint64_t value)
{
	unsigned int i;

	for (i = 0; i < 8; i++)
		data[7U - i] = (uint8_t)(value >> (i * 8U));
}

static bool scsi_lba_needs_16(uint64_t lba, uint16_t blocks)
{
	/* Match the native UFS scan path, which switches to READ/WRITE(16)
	 * above SCSI_LBA48_READ (0x0fffffff), even though READ(10) can encode
	 * the full 32-bit LBA range. */
	return lba > UFS_SCSI_READ16_THRESHOLD || (uint64_t)(blocks - 1U) > UINT32_MAX - lba;
}

int ufs_scsi_exec(
	struct ufs_scsi_device *device, const uint8_t *cdb, uint8_t cdb_len, void *data, size_t data_len, bool write)
{
	struct ufshc_request request;
	int ret;

	if (!device || !device->host || !cdb || !cdb_len || cdb_len > UFSHC_CDB_SIZE)
		return UFS_SCSI_ERR;
	memset(&request, 0, sizeof(request));
	device->last_response_type = 0;
	device->last_task_response = 0;
	device->last_status = 0;
	device->last_sense_length = 0;
	device->last_residual_transfer_count = 0;
	request.lun = device->lun;
	request.cdb_len = cdb_len;
	request.data = data;
	request.data_len = data_len;
	request.write = write;
	memcpy(request.cdb, cdb, cdb_len);
	ret = ufshc_exec(device->host, &request);
	device->last_response_type = request.response_type;
	device->last_task_response = request.task_response;
	device->last_status = request.status;
	device->last_sense_length = request.sense_length;
	device->last_residual_transfer_count = request.residual_transfer_count;
	if (request.sense_length)
		memcpy(device->last_sense, request.sense, request.sense_length);
	/* A CHECK CONDITION response carries sense data in the UPIU on most
	 * devices.  A few UFS parts only return a valid sense buffer after the
	 * traditional REQUEST SENSE command, so complete the SCSI error sequence
	 * here while retaining the original command status for the caller. */
	if (ret && request.status == UFS_SCSI_CHECK_CONDITION && cdb[0] != UFS_SCSI_REQUEST_SENSE) {
		uint8_t response_type = device->last_response_type;
		uint8_t task_response = device->last_task_response;
		uint8_t status = device->last_status;
		uint32_t residual_transfer_count = device->last_residual_transfer_count;
		uint8_t sense_length = device->last_sense_length;
		uint8_t sense[UFSHC_SCSI_SENSE_SIZE];
		int sense_ret;

		memcpy(sense, device->last_sense, sizeof(sense));
		sense_ret = ufs_scsi_request_sense(device);
		if (sense_ret) {
			device->last_sense_length = sense_length;
			memcpy(device->last_sense, sense, sizeof(sense));
		}
		device->last_response_type = response_type;
		device->last_task_response = task_response;
		device->last_status = status;
		device->last_residual_transfer_count = residual_transfer_count;
	}
	return ret;
}

static int ufs_scsi_inquiry(struct ufs_scsi_device *device)
{
	uint8_t cdb[6] = { UFS_SCSI_INQUIRY, (uint8_t)(device->lun << 5), 0, 0, UFS_SCSI_INQUIRY_DATA_LENGTH, 0 };
	uint8_t inquiry[UFS_SCSI_INQUIRY_DATA_LENGTH];
	size_t model_end;

	memset(inquiry, 0, sizeof(inquiry));
	if (ufs_scsi_exec(device, cdb, sizeof(cdb), inquiry, sizeof(inquiry), false))
		return UFS_SCSI_ERR;
	if ((inquiry[0] & 0x1fU) == 0x1fU)
		return UFS_SCSI_ERR;
	/* The model field is fixed-width and space padded by SCSI. */
	model_end = 16U + 16U;
	while (model_end > 16U && inquiry[model_end - 1U] == ' ')
		--model_end;
	if (model_end - 16U >= sizeof(device->model))
		model_end = 16U + sizeof(device->model) - 1U;
	memcpy(device->model, &inquiry[16], model_end - 16U);
	device->model[model_end - 16U] = '\0';
	return 0;
}

int ufs_scsi_test_unit_ready(struct ufs_scsi_device *device)
{
	uint8_t cdb[6] = { UFS_SCSI_TEST_UNIT_READY, 0, 0, 0, 0, 0 };

	if (!device)
		return UFS_SCSI_ERR;
	cdb[1] = (uint8_t)(device->lun << 5);

	return ufs_scsi_exec(device, cdb, sizeof(cdb), NULL, 0, false);
}

int ufs_scsi_request_sense(struct ufs_scsi_device *device)
{
	uint8_t cdb[6] = { UFS_SCSI_REQUEST_SENSE, 0, 0, 0, UFS_SCSI_SENSE_TRANSFER_LENGTH, 0 };
	uint8_t sense[UFS_SCSI_SENSE_TRANSFER_LENGTH] __attribute__((aligned(4)));
	uint32_t valid_length;
	int ret;

	if (!device || !device->host)
		return UFS_SCSI_ERR;
	cdb[1] = (uint8_t)(device->lun << 5);
	memset(sense, 0, sizeof(sense));
	ret = ufs_scsi_exec(device, cdb, sizeof(cdb), sense, sizeof(sense), false);
	if (ret)
		return ret;
	valid_length = sizeof(sense);
	if (device->last_residual_transfer_count >= valid_length)
		valid_length = 0;
	else
		valid_length -= device->last_residual_transfer_count;
	/* REQUEST SENSE is allowed to return fewer than the requested bytes;
	 * preserve only the standard sense payload. */
	if (valid_length > UFSHC_SCSI_SENSE_SIZE)
		valid_length = UFSHC_SCSI_SENSE_SIZE;
	memset(device->last_sense, 0, sizeof(device->last_sense));
	memcpy(device->last_sense, sense, valid_length);
	device->last_sense_length = (uint8_t)valid_length;
	return 0;
}

static int ufs_scsi_start_stop(struct ufs_scsi_device *device)
{
	/* Start the logical unit and request immediate return, as in the native
	 * Allwinner scan flow when TUR reports a transient not-ready condition. */
	uint8_t cdb[6] = { UFS_SCSI_START_STOP, 0, 0, 0, 1, 0 };

	cdb[1] = (uint8_t)((device->lun << 5) | 1U);

	return ufs_scsi_exec(device, cdb, sizeof(cdb), NULL, 0, false);
}

static int ufs_scsi_read_capacity(struct ufs_scsi_device *device)
{
	uint8_t cdb[16] = { 0 };
	uint8_t capacity[UFS_SCSI_CAPACITY16_LENGTH];
	uint32_t last_lba;
	uint32_t block_size;

	cdb[0] = UFS_SCSI_READ_CAPACITY10;
	cdb[1] = (uint8_t)(device->lun << 5);
	memset(capacity, 0, sizeof(capacity));
	if (ufs_scsi_exec(device, cdb, 10, capacity, 8, false))
		return UFS_SCSI_ERR;
	last_lba = scsi_be32(capacity);
	block_size = scsi_be32(&capacity[4]);
	if (!block_size)
		return UFS_SCSI_ERR;

	if (last_lba == 0xffffffffU) {
		memset(cdb, 0, sizeof(cdb));
		memset(capacity, 0, sizeof(capacity));
		cdb[0] = UFS_SCSI_READ_CAPACITY16;
		cdb[1] = (uint8_t)((device->lun << 5) | 0x10U); /* service action: READ CAPACITY(16) */
		scsi_set_be32(&cdb[10], UFS_SCSI_CAPACITY16_LENGTH);
		if (ufs_scsi_exec(device, cdb, 16, capacity, sizeof(capacity), false))
			return UFS_SCSI_ERR;
		if (scsi_be64(capacity) == UINT64_MAX)
			return UFS_SCSI_ERR;
		device->block_count = scsi_be64(capacity) + 1U;
		block_size = scsi_be32(&capacity[8]);
		if (!block_size)
			return UFS_SCSI_ERR;
	} else {
		device->block_count = (uint64_t)last_lba + 1U;
	}
	device->block_size = block_size;
	return 0;
}

int ufs_scsi_init(struct ufs_scsi_device *device, struct ufshc_host *host, uint8_t lun)
{
	unsigned int retry;
	bool started = false;
	int ret;

	if (!device || !host || lun > UFS_SCSI_MAX_LUN)
		return UFS_SCSI_ERR;
	memset(device, 0, sizeof(*device));
	device->host = host;
	device->lun = lun;
	for (retry = 0; retry < 3U; ++retry) {
		ret = ufs_scsi_inquiry(device);
		if (!ret)
			break;
	}
	if (ret)
		goto not_ready;
	for (retry = 0; retry < 3U; ++retry) {
		unsigned int attempt;

		for (attempt = 0; attempt < 3U; ++attempt) {
			ret = ufs_scsi_test_unit_ready(device);
			if (!ret)
				break;
		}
		if (!ret)
			break;
		/* Native UFS scan sends START STOP only after the first failed TUR;
		 * subsequent groups simply allow the logical unit time to settle. */
		if (!started) {
			(void)ufs_scsi_start_stop(device);
			started = true;
		}
		if (retry + 1U < 3U)
			mdelay(1000);
	}
	if (ret || ufs_scsi_read_capacity(device)) {
not_ready:
		printk_error("UFS: SCSI logical unit %u is not ready\n", lun);
		return UFS_SCSI_ERR;
	}
	device->present = true;
	printk_info("UFS: LUN %u, %s, %llu blocks x %u bytes\n", lun, device->model[0] ? device->model : "unknown",
		(unsigned long long)device->block_count, device->block_size);
	return 0;
}

int ufs_scsi_read(struct ufs_scsi_device *device, uint64_t lba, uint32_t blocks, void *buffer)
{
	uint8_t cdb[16];
	uint8_t *cursor = (uint8_t *)buffer;

	if (!device || !device->present)
		return UFS_SCSI_ERR;
	if (!blocks)
		return 0;
	if (!buffer || lba >= device->block_count || blocks > device->block_count - lba)
		return UFS_SCSI_ERR;
	while (blocks) {
		uint16_t count = blocks > UFS_SCSI_MAX_BLOCKS ? UFS_SCSI_MAX_BLOCKS : (uint16_t)blocks;
		uint8_t cdb_len;
		memset(cdb, 0, sizeof(cdb));
		if (scsi_lba_needs_16(lba, count)) {
			cdb[0] = UFS_SCSI_READ16;
			cdb[1] = (uint8_t)(device->lun << 5);
			scsi_set_be64(&cdb[2], lba);
			scsi_set_be32(&cdb[10], count);
			cdb_len = 16;
		} else {
			cdb[0] = UFS_SCSI_READ10;
			cdb[1] = (uint8_t)(device->lun << 5);
			scsi_set_be32(&cdb[2], (uint32_t)lba);
			cdb[7] = (uint8_t)(count >> 8);
			cdb[8] = (uint8_t)count;
			cdb_len = 10;
		}
		if (device->block_size > (size_t)-1 / count ||
			ufs_scsi_exec(device, cdb, cdb_len, cursor, (size_t)count * device->block_size, false))
			return UFS_SCSI_ERR;
		lba += count;
		blocks -= count;
		cursor += (size_t)count * device->block_size;
	}
	return 0;
}

int ufs_scsi_write(struct ufs_scsi_device *device, uint64_t lba, uint32_t blocks, const void *buffer)
{
	uint8_t cdb[16];
	const uint8_t *cursor = (const uint8_t *)buffer;

	if (!device || !device->present)
		return UFS_SCSI_ERR;
	if (!blocks)
		return 0;
	if (!buffer || lba >= device->block_count || blocks > device->block_count - lba)
		return UFS_SCSI_ERR;
	while (blocks) {
		uint16_t count = blocks > UFS_SCSI_MAX_BLOCKS ? UFS_SCSI_MAX_BLOCKS : (uint16_t)blocks;
		uint8_t cdb_len;
		memset(cdb, 0, sizeof(cdb));
		if (scsi_lba_needs_16(lba, count)) {
			cdb[0] = UFS_SCSI_WRITE16;
			cdb[1] = (uint8_t)(device->lun << 5);
			scsi_set_be64(&cdb[2], lba);
			scsi_set_be32(&cdb[10], count);
			cdb_len = 16;
		} else {
			cdb[0] = UFS_SCSI_WRITE10;
			cdb[1] = (uint8_t)(device->lun << 5);
			scsi_set_be32(&cdb[2], (uint32_t)lba);
			cdb[7] = (uint8_t)(count >> 8);
			cdb[8] = (uint8_t)count;
			cdb_len = 10;
		}
		if (device->block_size > (size_t)-1 / count ||
			ufs_scsi_exec(device, cdb, cdb_len, (void *)cursor, (size_t)count * device->block_size, true))
			return UFS_SCSI_ERR;
		lba += count;
		blocks -= count;
		cursor += (size_t)count * device->block_size;
	}
	return 0;
}
