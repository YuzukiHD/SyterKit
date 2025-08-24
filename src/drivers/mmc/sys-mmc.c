/* SPDX-License-Identifier:	GPL-2.0+ */

#include <barrier.h>
#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <timer.h>

#include <sys-clk.h>
#include <sys-gpio.h>

#include <mmc/sys-mmc.h>
#include <mmc/sys-sdhci.h>

/**
 * @brief Extracts a specified bit field from a response buffer.
 *
 * This macro extracts a specified bit field from a response buffer or an array of integers.
 * It calculates the offset, shift amount, and bitmask based on the starting position and size of the field,
 * then performs the extraction and applies the bitmask to isolate the desired bits.
 *
 * @param resp The response buffer or array of integers.
 * @param start The starting bit position of the field within the response buffer.
 * @param size The size (in bits) of the field to be extracted.
 * @return The extracted bit field.
 */
#define UNSTUFF_BITS(resp, start, size)                              \
	({                                                               \
		const int __size = size;                                     \
		const uint32_t __mask = (__size < 32 ? 1 << __size : 0) - 1; \
		const int __off = 3 - ((start) / 32);                        \
		const int __shft = (start) &31;                              \
		uint32_t __res;                                              \
                                                                     \
		__res = resp[__off] >> __shft;                               \
		if (__size + __shft > 32)                                    \
			__res |= resp[__off - 1] << ((32 - __shft) % 32);        \
		__res &__mask;                                               \
	})


/**
 * @brief Checks if the MMC host operates in SPI mode.
 *
 * This function checks if the MMC host operates in SPI mode based on its capabilities.
 *
 * @param mmc Pointer to the MMC structure.
 * @return 1 if the host operates in SPI mode, 0 otherwise.
 */
static inline int sunxi_mmc_host_is_spi(mmc_t *mmc) {
	return mmc->host_caps & MMC_MODE_SPI;
}

/**
 * @brief Checks if the MMC device is an SD card.
 *
 * This function checks if the MMC device is an SD card based on its version information.
 *
 * @param mmc Pointer to the MMC structure.
 * @return 1 if the device is an SD card, 0 otherwise.
 */
static inline int sunxi_mmc_device_is_sd(mmc_t *mmc) {
	return mmc->version & SD_VERSION_SD;
}

/**
 * @brief Extracts the Manufacturer ID from the MMC card.
 *
 * This function extracts the Manufacturer ID from the MMC card's CID (Card Identification Data) register.
 * The extraction is performed based on the MMC version and CID structure.
 *
 * @param card Pointer to the MMC card structure.
 * @return The Manufacturer ID.
 */
static inline uint32_t extract_mid(mmc_t *card) {
	if ((card->version & MMC_VERSION_MMC) && (card->version <= MMC_VERSION_1_4))
		return UNSTUFF_BITS(card->cid, 104, 24);
	else
		return UNSTUFF_BITS(card->cid, 120, 8);
}

/**
 * @brief Extracts the OEM/Application ID from the MMC card.
 *
 * This function extracts the OEM/Application ID from the MMC card's CID (Card Identification Data) register.
 *
 * @param card Pointer to the MMC card structure.
 * @return The OEM/Application ID.
 */
static inline uint32_t extract_oid(mmc_t *card) {
	return (card->cid[0] >> 8) & 0xffff;
}

/**
 * @brief Extracts the Product Revision from the MMC card.
 *
 * This function extracts the Product Revision from the MMC card's CID (Card Identification Data) register.
 *
 * @param card Pointer to the MMC card structure.
 * @return The Product Revision.
 */
static inline uint32_t extract_prv(mmc_t *card) {
	return (card->cid[2] >> 24);
}

/**
 * @brief Extracts the Product Serial Number (PSN) from the MMC card.
 *
 * This function extracts the Product Serial Number (PSN) from the MMC card based on its version and CSD/CID structure.
 *
 * @param card Pointer to the MMC card structure.
 * @return The Product Serial Number.
 */
static inline uint32_t extract_psn(mmc_t *card) {
	if (card->version & SD_VERSION_SD) {
		return UNSTUFF_BITS(card->csd, 24, 32);
	} else {
		if (card->version > MMC_VERSION_1_4)
			return UNSTUFF_BITS(card->cid, 16, 32);
		else
			return UNSTUFF_BITS(card->cid, 16, 24);
	}
}

/**
 * @brief Extracts the manufacturing month from the MMC card.
 *
 * This function extracts the manufacturing month from the MMC card's CID (Card Identification Data) register.
 *
 * @param card Pointer to the MMC card structure.
 * @return The manufacturing month.
 */
static inline uint32_t extract_month(mmc_t *card) {
	if (card->version & SD_VERSION_SD)
		return UNSTUFF_BITS(card->cid, 8, 4);
	else
		return UNSTUFF_BITS(card->cid, 12, 4);
}

/**
 * @brief Extracts the manufacturing year from the MMC card.
 *
 * This function extracts the manufacturing year from the MMC card's CID (Card Identification Data) register.
 *
 * @param card Pointer to the MMC card structure.
 * @return The manufacturing year.
 */
static inline uint32_t extract_year(mmc_t *card) {
	uint32_t year;

	if (card->version & SD_VERSION_SD)
		year = UNSTUFF_BITS(card->cid, 12, 8) + 2000;
	else if (card->version < MMC_VERSION_4_41)
		return UNSTUFF_BITS(card->cid, 8, 4) + 1997;
	else {
		year = UNSTUFF_BITS(card->cid, 8, 4) + 1997;
		if (year < 2010)
			year += 16;
	}
	return year;
}
/**
 * @brief Sends status command to the SD/MMC card and waits for the card to be ready.
 *
 * This function sends the status command to the SD/MMC card and waits for the card to be ready for data transfer.
 * It retries sending the command until the card is ready or until the timeout expires.
 *
 * @param sdhci Pointer to the SDHCI controller structure.
 * @param timeout Timeout value in milliseconds.
 * @return 0 on success, error code otherwise.
 */
static int sunxi_mmc_send_status(sunxi_sdhci_t *sdhci, uint32_t timeout) {
	mmc_t *mmc = sdhci->mmc;
	int err = 0;

	mmc_cmd_t cmd;
	cmd.cmdidx = MMC_CMD_SEND_STATUS;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = mmc->rca << 16;
	cmd.flags = 0;

	do {
		err = sunxi_sdhci_xfer(sdhci, &cmd, NULL);
		if (err) {
			printk_warning("SMHC%u: Send status failed\n", sdhci->id);
			return err;
		} else if (cmd.response[0] & MMC_STATUS_RDY_FOR_DATA)
			break;
		mdelay(1);
		if (cmd.response[0] & MMC_STATUS_MASK) {
			printk_warning("SMHC%u: Status Error: 0x%08X\n", sdhci->id, cmd.response[0]);
			return COMM_ERR;
		}
	} while (timeout--);

	if (!timeout) {
		printk_warning("SMHC%u: Timeout waiting card ready\n", sdhci->id);
		return TIMEOUT;
	}

	return 0;
}

/**
 * @brief Sets the block length for data transfer on the SD/MMC card.
 *
 * This function sets the block length for data transfer on the SD/MMC card, except in DDR mode.
 *
 * @param sdhci Pointer to the SDHCI controller structure.
 * @param len Block length to be set.
 * @return 0 on success, error code otherwise.
 */
static int sunxi_mmc_set_block_len(sunxi_sdhci_t *sdhci, uint32_t len) {
	mmc_t *mmc = sdhci->mmc;
	mmc_cmd_t cmd;
	/* Don't set block length in DDR mode */
	if ((mmc->speed_mode == MMC_HSDDR52_DDR50) || (mmc->speed_mode == MMC_HS400)) {
		return 0;
	}
	cmd.cmdidx = MMC_CMD_SET_BLOCKLEN;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = len;
	cmd.flags = 0;

	return sunxi_sdhci_xfer(sdhci, &cmd, NULL);
}

/**
 * @brief Reads blocks from the SD/MMC card.
 *
 * This function reads blocks from the SD/MMC card starting from the specified block address.
 * It supports reading multiple blocks and handles high capacity cards appropriately.
 *
 * @param sdhci Pointer to the SDHCI controller structure.
 * @param dst Pointer to the destination buffer where the data will be stored.
 * @param start Start block address from where to read the data.
 * @param blkcnt Number of blocks to read.
 * @return Number of blocks read on success, 0 otherwise.
 */
static uint32_t sunxi_mmc_read_blocks(sunxi_sdhci_t *sdhci, void *dst, uint32_t start, uint32_t blkcnt) {
	mmc_t *mmc = sdhci->mmc;

	mmc_cmd_t cmd = {0};
	mmc_data_t data = {0};

	int timeout = 1000;

	if (blkcnt > 1UL)
		cmd.cmdidx = MMC_CMD_READ_MULTIPLE_BLOCK;
	else
		cmd.cmdidx = MMC_CMD_READ_SINGLE_BLOCK;

	if (mmc->high_capacity)
		cmd.cmdarg = start;
	else
		cmd.cmdarg = start * mmc->read_bl_len;

	cmd.resp_type = MMC_RSP_R1;
	cmd.flags = 0;

	data.b.dest = dst;
	data.blocks = blkcnt;
	data.blocksize = mmc->read_bl_len;
	data.flags = MMC_DATA_READ;

	if (sunxi_sdhci_xfer(sdhci, &cmd, &data)) {
		printk_warning("SMHC: read block failed\n");
		return 0;
	}

	if (blkcnt > 1) {
		cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		cmd.cmdarg = 0;
		cmd.resp_type = MMC_RSP_R1b;
		cmd.flags = 0;
		if (sunxi_sdhci_xfer(sdhci, &cmd, NULL)) {
			printk_warning("SMHC: failed to send stop command\n");
			return 0;
		}

		/* Waiting for the ready status */
		sunxi_mmc_send_status(sdhci, timeout);
	}

	return blkcnt;
}

/**
 * @brief Writes blocks of data to the MMC device.
 *
 * This function writes a specified number of blocks to the MMC (MultiMediaCard) device
 * associated with the given SDHCI (SD Host Controller Interface) instance. It supports both
 * single block and multiple block write operations, and handles sending the appropriate
 * commands to the MMC device.
 *
 * @param[in] sdhci A pointer to the SDHCI instance.
 * @param[in] dst A pointer to the destination buffer from which data will be written to the MMC.
 * @param[in] start The starting block number where the data writing begins.
 * @param[in] blkcnt The number of blocks to write.
 *
 * @return The number of blocks successfully written, or 0 if writing failed.
 */
static uint32_t sunxi_mmc_write_blocks(sunxi_sdhci_t *sdhci, void *dst, uint32_t start, uint32_t blkcnt) {
	mmc_t *mmc = sdhci->mmc;

	mmc_cmd_t cmd = {0};
	mmc_data_t data = {0};

	int timeout = 1000;

	if (blkcnt > 1UL)
		cmd.cmdidx = MMC_CMD_WRITE_MULTIPLE_BLOCK;
	else
		cmd.cmdidx = MMC_CMD_WRITE_SINGLE_BLOCK;

	if (mmc->high_capacity)
		cmd.cmdarg = start;
	else
		cmd.cmdarg = start * mmc->write_bl_len;

	cmd.resp_type = MMC_RSP_R1;
	cmd.flags = 0;

	data.b.dest = dst;
	data.blocks = blkcnt;
	data.blocksize = mmc->write_bl_len;
	data.flags = MMC_DATA_WRITE;

	if (sunxi_sdhci_xfer(sdhci, &cmd, &data)) {
		printk_warning("SMHC: read block failed\n");
		return 0;
	}

	if (blkcnt > 1) {
		cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		cmd.cmdarg = 0;
		cmd.resp_type = MMC_RSP_R1b;
		cmd.flags = 0;
		if (sunxi_sdhci_xfer(sdhci, &cmd, NULL)) {
			printk_warning("SMHC: failed to send stop command\n");
			return 0;
		}

		/* Waiting for the ready status */
		sunxi_mmc_send_status(sdhci, timeout);
	}

	return blkcnt;
}

/**
 * @brief Sends the SD/MMC card to idle state.
 *
 * This function sends the SD/MMC card to the idle state, preparing it for further commands.
 *
 * @param sdhci Pointer to the SDHCI controller structure.
 * @return 0 on success, error code otherwise.
 */
static int sunxi_mmc_go_idle(sunxi_sdhci_t *sdhci) {
	mmc_t *mmc = sdhci->mmc;
	mmc_cmd_t cmd;

	int err = 0;

	cmd.cmdidx = MMC_CMD_GO_IDLE_STATE;
	cmd.cmdarg = 0;
	cmd.resp_type = MMC_RSP_NONE;
	cmd.flags = 0;

	err = sunxi_sdhci_xfer(sdhci, &cmd, NULL);

	if (err) {
		printk_warning("SMHC: idle failed\n");
		return err;
	}
	mdelay(2);
	return 0;
}
/**
 * @brief Sends SD card initialization sequence and waits for it to become ready.
 *
 * This function sends the SD card initialization sequence, which includes sending
 * application-specific commands and checking the card's response until it becomes ready.
 * It also updates the MMC structure with relevant information such as the card version,
 * OCR value, and high capacity flag.
 *
 * @param sdhci Pointer to the SDHCI controller structure.
 * @return 0 on success, error code otherwise.
 */
static int sunxi_mmc_sd_send_op_cond(sunxi_sdhci_t *sdhci) {
	int timeout = 1000;// Timeout value for waiting on card initialization
	int err;		   // Error code variable

	mmc_t *mmc = sdhci->mmc;// MMC structure pointer
	mmc_cmd_t cmd;			// MMC command structure

	do {
		// Send application-specific command
		cmd.cmdidx = MMC_CMD_APP_CMD;
		cmd.resp_type = MMC_RSP_R1;
		cmd.cmdarg = 0;
		cmd.flags = 0;

		// Transfer the command and check for errors
		err = sunxi_sdhci_xfer(sdhci, &cmd, NULL);

		if (err) {
			printk_warning("SMHC: send app cmd failed\n");
			return err;
		}

		// Send SD card operation condition command
		cmd.cmdidx = SD_CMD_APP_SEND_OP_COND;
		cmd.resp_type = MMC_RSP_R3;

		// Set command arguments based on card type and version
		cmd.cmdarg = sunxi_mmc_host_is_spi(mmc) ? 0 : (mmc->voltages & 0xff8000);

		if (mmc->version == SD_VERSION_2)
			cmd.cmdarg |= OCR_HCS;

		// Transfer the command and check for errors
		err = sunxi_sdhci_xfer(sdhci, &cmd, NULL);

		if (err) {
			printk_warning("SMHC: send cmd41 failed\n");
			return err;
		}

		mdelay(1);// Delay for stability

	} while ((!(cmd.response[0] & OCR_BUSY)) && timeout--);// Wait for card initialization and decrement timeout

	if (timeout <= 0) {
		printk_warning("SMHC: wait card init failed\n");
		return UNUSABLE_ERR;
	}

	// Update MMC structure with card information
	if (mmc->version != SD_VERSION_2)
		mmc->version = SD_VERSION_1_0;

	if (sunxi_mmc_host_is_spi(mmc)) {
		// For SPI, read OCR value
		cmd.cmdidx = MMC_CMD_SPI_READ_OCR;
		cmd.resp_type = MMC_RSP_R3;
		cmd.cmdarg = 0;
		cmd.flags = 0;

		// Transfer the command and check for errors
		err = sunxi_sdhci_xfer(sdhci, &cmd, NULL);

		if (err) {
			printk_warning("SMHC: spi read ocr failed\n");
			return err;
		}
	}

	// Update MMC structure with OCR value, high capacity flag, and relative card address
	mmc->ocr = cmd.response[0];
	mmc->high_capacity = ((mmc->ocr & OCR_HCS) == OCR_HCS);
	mmc->rca = 0;

	return 0;// Return success
}

/**
 * @brief Send the SEND_OP_COND command to the MMC/SD card.
 *
 * This function sends the SEND_OP_COND command to the MMC/SD card to initialize and
 * check its capabilities. It waits for the card to respond and initializes it, updating
 * the MMC structure with card information upon successful initialization.
 *
 * @param sdhci     Pointer to the Sunxi SDHCI controller structure.
 *
 * @return          Returns 0 on success, indicating successful initialization of the card,
 *                  or an error code indicating failure.
 *                  Possible error codes:
 *                      - Negative value: indicates a communication error with the MMC/SD card.
 *                      - Positive value: indicates an internal error within the function.
 *                      - UNUSABLE_ERR: indicates failure to initialize the card within the timeout.
 */
static int sunxi_mmc_mmc_send_op_cond(sunxi_sdhci_t *sdhci) {
	int timeout = 1000;		///< Timeout value for waiting for card initialization.
	int err;				///< Error code for indicating success or failure.
	mmc_t *mmc = sdhci->mmc;///< Pointer to the MMC structure.
	mmc_cmd_t cmd;			///< Command structure for the SEND_OP_COND command.

	// Reset the MMC/SD card
	sunxi_mmc_go_idle(sdhci);

	// Send the SEND_OP_COND command to inquire about card capabilities
	cmd.cmdidx = MMC_CMD_SEND_OP_COND;
	cmd.resp_type = MMC_RSP_R3;
	cmd.cmdarg = 0;
	cmd.flags = 0;

	// Send command to check card capabilities
	err = sunxi_sdhci_xfer(sdhci, &cmd, NULL);
	if (err) {
		printk_warning("SMHC: send op cond failed\n");
		return err;
	}

	mdelay(1);// Delay for stability

	// Loop until the card is initialized or timeout occurs
	do {
		cmd.cmdidx = MMC_CMD_SEND_OP_COND;
		cmd.resp_type = MMC_RSP_R3;

		// Set command arguments based on card type and version
		cmd.cmdarg = (sunxi_mmc_host_is_spi(mmc) ? 0 : (mmc->voltages & (cmd.response[0] & OCR_VOLTAGE_MASK)) | (cmd.response[0] & OCR_ACCESS_MODE));
		if (mmc->host_caps & MMC_MODE_HC)
			cmd.cmdarg |= OCR_HCS;
		cmd.flags = 0;

		// Send command to check card capabilities
		err = sunxi_sdhci_xfer(sdhci, &cmd, NULL);
		if (err) {
			printk_warning("SMHC: send op cond failed\n");
			return err;
		}

		mdelay(1);// Delay for stability

	} while (!(cmd.response[0] & OCR_BUSY) && timeout--);// Wait for card initialization and decrement timeout

	if (timeout <= 0) {
		printk_warning("SMHC: wait for mmc init failed\n");
		return UNUSABLE_ERR;// Indicate failure to initialize the card within the timeout
	}

	// Read OCR for SPI
	if (sunxi_mmc_host_is_spi(mmc)) {
		cmd.cmdidx = MMC_CMD_SPI_READ_OCR;
		cmd.resp_type = MMC_RSP_R3;
		cmd.cmdarg = 0;
		cmd.flags = 0;

		err = sunxi_sdhci_xfer(sdhci, &cmd, NULL);
		if (err)
			return err;
	}

	// Update MMC structure with card information
	mmc->version = MMC_VERSION_UNKNOWN;
	mmc->ocr = cmd.response[0];
	mmc->high_capacity = ((mmc->ocr & OCR_HCS) == OCR_HCS);
	mmc->rca = 1;

	return 0;// Return success
}

/**
 * @brief Send the SEND_EXT_CSD command to retrieve the Extended CSD from the MMC/SD card.
 *
 * This function sends the SEND_EXT_CSD command to the MMC/SD card to retrieve its Extended CSD register,
 * which contains various configuration parameters and settings. It stores the retrieved Extended CSD data
 * in the provided buffer.
 *
 * @param sdhci     Pointer to the Sunxi SDHCI controller structure.
 * @param ext_csd   Pointer to the buffer where the Extended CSD data will be stored.
 *
 * @return          Returns 0 on success, indicating successful retrieval of the Extended CSD data,
 *                  or an error code indicating failure.
 *                  Possible error codes:
 *                      - Negative value: indicates a communication error with the MMC/SD card.
 *                      - Positive value: indicates an internal error within the function.
 */
static int sunxi_mmc_send_ext_csd(sunxi_sdhci_t *sdhci, char *ext_csd) {
	mmc_t *mmc = sdhci->mmc;///< Pointer to the MMC structure.
	mmc_cmd_t cmd;			///< Command structure for the SEND_EXT_CSD command.
	mmc_data_t data;		///< Data structure for storing the Extended CSD data.
	int err;				///< Error code for indicating success or failure.

	// Send command to retrieve the Extended CSD
	cmd.cmdidx = MMC_CMD_SEND_EXT_CSD;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = 0;
	cmd.flags = 0;

	// Prepare data structure to store the Extended CSD
	data.b.dest = ext_csd;
	data.blocks = 1;
	data.blocksize = 512;
	data.flags = MMC_DATA_READ;

	// Send command to retrieve the Extended CSD and store it in the provided buffer
	err = sunxi_sdhci_xfer(sdhci, &cmd, &data);

	if (err)
		printk_warning("SMHC: send ext csd failed\n");

	return err;// Return the error code (0 if successful)
}

/**
 * @brief Send the SWITCH command to the MMC/SD card to change a specified mode or setting.
 *
 * This function sends the SWITCH command to the MMC/SD card to change a specified mode or setting.
 * It is typically used to modify various parameters or configurations of the card.
 *
 * @param sdhci     Pointer to the Sunxi SDHCI controller structure.
 * @param set       Value indicating the type of setting to be changed.
 * @param index     Index of the setting within the specified set.
 * @param value     New value to set for the specified setting.
 *
 * @return          Returns 0 on success, indicating successful completion of the SWITCH command,
 *                  or an error code indicating failure.
 *                  Possible error codes:
 *                      - Negative value: indicates a communication error with the MMC/SD card.
 *                      - Positive value: indicates an internal error within the function.
 */
static int sunxi_mmc_switch(sunxi_sdhci_t *sdhci, uint8_t set, uint8_t index, uint8_t value) {
	mmc_t *mmc = sdhci->mmc;///< Pointer to the MMC structure.
	mmc_cmd_t cmd;			///< Command structure for the SWITCH command.
	int timeout = 1000;		///< Timeout value for waiting for the card to become ready.
	int ret;				///< Error code for indicating success or failure.

	// Prepare the SWITCH command
	cmd.cmdidx = MMC_CMD_SWITCH;
	cmd.resp_type = MMC_RSP_R1b;
	cmd.cmdarg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) | (index << 16) | (value << 8) | set;
	cmd.flags = 0;

	// Send the SWITCH command to the card
	ret = sunxi_sdhci_xfer(sdhci, &cmd, NULL);
	if (ret) {
		printk_warning("SMHC: switch failed\n");
	}

	/* for re-update sample phase */
	// Update clock phase after sending command 6
	ret = sunxi_sdhci_update_phase(sdhci);
	if (ret) {
		printk_warning("SMHC: update clock failed after send cmd6\n");
		return ret;
	}

	/* Waiting for the ready status */
	// Wait for the card to become ready
	sunxi_mmc_send_status(sdhci, timeout);

	return ret;// Return the error code (0 if successful)
}

/**
 * @brief Change the frequency of the MMC/SD card to support high-speed modes.
 *
 * This function changes the frequency of the MMC/SD card to support high-speed modes.
 * It checks if the card supports high-speed modes and switches to the appropriate mode if possible.
 *
 * @param sdhci     Pointer to the Sunxi SDHCI controller structure.
 *
 * @return          Returns 0 on success, indicating successful completion of the frequency change,
 *                  or an error code indicating failure.
 *                  Possible error codes:
 *                      - Negative value: indicates a communication error with the MMC/SD card.
 *                      - Positive value: indicates an internal error within the function.
 */
static int sunxi_mmc_mmc_change_freq(sunxi_sdhci_t *sdhci) {
	mmc_t *mmc = sdhci->mmc;///< Pointer to the MMC structure.
	char ext_csd[512];		///< Buffer to hold the extended CSD data.
	char cardtype;			///< Type of the MMC/SD card.
	int err;				///< Error code for indicating success or failure.
	int retry = 5;			///< Number of retries for certain operations.

	mmc->card_caps = 0;///< Initialize card capabilities.

	// Skip frequency change if MMC/SD card is in SPI mode
	if (sunxi_mmc_host_is_spi(mmc))
		return 0;

	// Check if the card version supports high-speed modes
	if (mmc->version < MMC_VERSION_4)
		return 0;

	// Enable 4-bit and 8-bit modes for MMC/SD card
	mmc->card_caps |= MMC_MODE_4BIT | MMC_MODE_8BIT;

	// Get the extended CSD data from the card
	err = sunxi_mmc_send_ext_csd(sdhci, ext_csd);
	if (err) {
		printk_warning("SMHC: get ext csd failed\n");
		return err;
	}

	cardtype = ext_csd[EXT_CSD_CARD_TYPE] & 0xff;///< Extract card type from the extended CSD data

	// Retry switching to high-speed mode for certain types of cards
	do {
		err = sunxi_mmc_switch(sdhci, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, 1);
		if (!err) {
			break;
		}
		printk_debug("SMHC: retry mmc switch(cmd6)\n");
	} while (retry--);

	if (err) {
		printk_warning("SMHC: change to hs failed\n");
		return err;
	}

	// Check if the frequency change was successful
	err = sunxi_mmc_send_ext_csd(sdhci, ext_csd);
	if (err) {
		printk_warning("SMHC: send ext csd faild\n");
		return err;
	}

	// Check if high-speed mode is supported
	if (!ext_csd[EXT_CSD_HS_TIMING])
		return 0;

	// Determine the type of high-speed mode and update card capabilities
	if (cardtype & EXT_CSD_CARD_TYPE_HS) {
		if (cardtype & EXT_CSD_CARD_TYPE_DDR_52) {
			printk_trace("SMHC: get ddr OK!\n");
			mmc->card_caps |= MMC_MODE_DDR_52MHz;
			mmc->speed_mode = MMC_HSDDR52_DDR50;
		} else {
			mmc->speed_mode = MMC_HSSDR52_SDR25;
		}
		mmc->card_caps |= MMC_MODE_HS_52MHz | MMC_MODE_HS;
	} else {
		mmc->card_caps |= MMC_MODE_HS;
		mmc->speed_mode = MMC_DS26_SDR12;
	}

	return 0;// Return success
}

/**
 * @brief Switch the functionality of the SD card.
 *
 * This function switches the functionality of the SD card, such as changing the frequency or mode of operation.
 * It sends a SWITCH_FUNC command to the SD card with the specified parameters to perform the desired switch.
 *
 * @param sdhci     Pointer to the Sunxi SDHCI controller structure.
 * @param mode      Switch mode indicating the type of switch operation to perform.
 * @param group     Switch group indicating the group of functions to switch.
 * @param value     Value indicating the specific function or setting to switch to within the specified group.
 * @param resp      Pointer to a buffer to store the response from the SD card.
 *
 * @return          Returns 0 on success, indicating successful completion of the switch operation,
 *                  or an error code indicating failure.
 *                  Possible error codes:
 *                      - Negative value: indicates a communication error with the SD card.
 *                      - Positive value: indicates an internal error within the function.
 */
static int sunxi_mmc_sd_switch(sunxi_sdhci_t *sdhci, int mode, int group, uint8_t value, uint8_t *resp) {
	mmc_cmd_t cmd;	///< MMC command structure for sending commands to the SD card.
	mmc_data_t data;///< MMC data structure for specifying data transfer parameters.

	/* Switch the frequency */
	cmd.cmdidx = SD_CMD_SWITCH_FUNC;				///< Command index for SWITCH_FUNC command.
	cmd.resp_type = MMC_RSP_R1;						///< Response type for the command.
	cmd.cmdarg = ((uint32_t) mode << 31) | 0xffffff;///< Command argument specifying the mode and reserved bits.
	cmd.cmdarg &= ~(0xf << (group * 4));			///< Clear the bits corresponding to the specified group.
	cmd.cmdarg |= value << (group * 4);				///< Set the bits corresponding to the specified value within the group.
	cmd.flags = 0;									///< Flags for command execution.

	data.b.dest = (char *) resp;///< Destination buffer for storing the response from the SD card.
	data.blocksize = 64;		///< Block size for data transfer.
	data.blocks = 1;			///< Number of blocks to transfer.
	data.flags = MMC_DATA_READ; ///< Flags indicating the direction of data transfer.

	// Execute the command and data transfer
	return sunxi_sdhci_xfer(sdhci, &cmd, &data);
}

/**
 * @brief Change the frequency of the SD card.
 *
 * This function changes the operating frequency of the SD card based on its capabilities.
 * It reads the SCR (SD Configuration Register) to determine if the card supports higher speeds.
 * If the card supports higher speeds, it adjusts the frequency accordingly.
 *
 * @param sdhci     Pointer to the Sunxi SDHCI controller structure.
 *
 * @return          Returns 0 on success, indicating successful change of frequency,
 *                  or an error code indicating failure.
 *                  Possible error codes:
 *                      - Negative value: indicates a communication error with the SD card.
 *                      - Positive value: indicates an internal error within the function.
 */
static int sunxi_mmc_sd_change_freq(sunxi_sdhci_t *sdhci) {
	mmc_t *mmc = sdhci->mmc;

	mmc_cmd_t cmd;
	mmc_data_t data;
	uint32_t scr[2];
	uint32_t switch_status[16];
	int err;
	int timeout;

	mmc->card_caps = 0;

	if (sunxi_mmc_host_is_spi(mmc))
		return 0;

	/* Read the SCR to find out if this card supports higher speeds */
	cmd.cmdidx = MMC_CMD_APP_CMD;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = mmc->rca << 16;
	cmd.flags = 0;

	err = sunxi_sdhci_xfer(sdhci, &cmd, NULL);

	if (err) {
		printk_warning("SMHC: Send app cmd failed\n");
		return err;
	}

	cmd.cmdidx = SD_CMD_APP_SEND_SCR;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = 0;
	cmd.flags = 0;

	timeout = 3;

retry_scr:
	data.b.dest = (char *) &scr;
	data.blocksize = 8;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;

	err = sunxi_sdhci_xfer(sdhci, &cmd, &data);

	if (err) {
		if (timeout--)
			goto retry_scr;
		printk_warning("SMHC: Send scr failed\n");
		return err;
	}

	mmc->scr[0] = be32_to_cpu(scr[0]);
	mmc->scr[1] = be32_to_cpu(scr[1]);

	switch ((mmc->scr[0] >> 24) & 0xf) {
		case 0:
			mmc->version = SD_VERSION_1_0;
			break;
		case 1:
			mmc->version = SD_VERSION_1_10;
			break;
		case 2:
			mmc->version = SD_VERSION_2;
			break;
		default:
			mmc->version = SD_VERSION_1_0;
			break;
	}

	if (mmc->scr[0] & SD_DATA_4BIT)
		mmc->card_caps |= MMC_MODE_4BIT;

	/* Version 1.0 doesn't support switching */
	if (mmc->version == SD_VERSION_1_0)
		return 0;

	timeout = 4;
	while (timeout--) {
		err = sunxi_mmc_sd_switch(sdhci, SD_SWITCH_CHECK, 0, 1, (uint8_t *) &switch_status);

		if (err) {
			printk_warning("SMHC: Check high speed status faild\n");
			return err;
		}

		/* The high-speed function is busy.  Try again */
		if (!(be32_to_cpu(switch_status[7]) & SD_HIGHSPEED_BUSY))
			break;
	}

	/* If high-speed isn't supported, we return */
	if (!(be32_to_cpu(switch_status[3]) & SD_HIGHSPEED_SUPPORTED))
		return 0;

	err = sunxi_mmc_sd_switch(sdhci, SD_SWITCH_SWITCH, 0, 1, (uint8_t *) &switch_status);

	if (err) {
		printk_warning("SMHC: switch to high speed failed\n");
		return err;
	}

	err = sunxi_sdhci_update_phase(sdhci);
	if (err) {
		printk_warning("SMHC: update clock failed after send cmd6 to switch to sd high speed mode\n");
		return err;
	}

	if ((be32_to_cpu(switch_status[4]) & 0x0f000000) == 0x01000000) {
		mmc->card_caps |= MMC_MODE_HS;
		mmc->speed_mode = MMC_HSSDR52_SDR25;
	}

	return 0;
}

/* frequency bases */
/* divided by 10 to be nice to platforms without floating point */
static const int tran_speed_unit[] = {
		10000,
		100000,
		1000000,
		10000000,
};

/* Multiplier values for TRAN_SPEED.  Multiplied by 10 to be nice
 * to platforms without floating point.
 */
static const int tran_speed_time[] = {
		0, /* reserved */
		10,
		12,
		13,
		15,
		20,
		25,
		30,
		35,
		40,
		45,
		50,
		55,
		60,
		70,
		80,
};

/**
 * @brief Set the clock frequency for the Sunxi SDHCI controller.
 * 
 * This function sets the clock frequency for the Secure Digital Host Controller Interface (SDHCI) in a Sunxi system-on-a-chip (SoC) environment.
 * 
 * @param sdhci A pointer to the Sunxi SDHCI controller structure.
 * @param clock The desired clock frequency to be set.
 */
static void sunxi_mmc_set_clock(sunxi_sdhci_t *sdhci, uint32_t clock) {
	mmc_t *mmc = sdhci->mmc;

	// Print debug information about clock frequencies
	printk_trace("SMHC: fmax:%u, fmin:%u, clk:%u\n", mmc->f_max, mmc->f_min, clock);

	// Ensure clock frequency is within supported range
	if (clock > mmc->f_max) {
		clock = mmc->f_max;
	}

	if (clock < mmc->f_min) {
		clock = mmc->f_min;
	}

	// Update MMC clock frequency
	mmc->clock = clock;

	// Apply new clock settings to SDHCI controller
	sunxi_sdhci_set_ios(sdhci);
}

/**
 * @brief Set the bus width for the Sunxi SDHCI controller.
 * 
 * This function sets the bus width for the Secure Digital Host Controller Interface (SDHCI) in a Sunxi system-on-a-chip (SoC) environment.
 * 
 * @param sdhci A pointer to the Sunxi SDHCI controller structure.
 * @param width The bus width to be set (in bits).
 */
static void sunxi_mmc_set_bus_width(sunxi_sdhci_t *sdhci, uint32_t width) {
	mmc_t *mmc = sdhci->mmc;

	// Set the bus width
	mmc->bus_width = width;

	// Apply new settings to SDHCI controller
	sunxi_sdhci_set_ios(sdhci);
}

/**
 * @brief Switch the Sunxi SDHCI controller to Double Speed (DS) mode.
 * 
 * This function switches the Secure Digital Host Controller Interface (SDHCI) in a Sunxi system-on-a-chip (SoC) environment to Double Speed (DS) mode.
 * 
 * @param sdhci A pointer to the Sunxi SDHCI controller structure.
 * @return Returns 0 on success, or a negative error code on failure.
 */
static int sunxi_mmc_mmc_switch_ds(sunxi_sdhci_t *sdhci) {
	mmc_t *mmc = sdhci->mmc;
	int err;

	// Check if already in SDR12 mode
	if (mmc->speed_mode == MMC_DS26_SDR12) {
		printk_trace("SMHC: set in SDR12 mode\n");
	}

	// Check if card supports DS mode
	if (!(mmc->card_caps && MMC_MODE_HS)) {
		printk_warning("SMHC: Card does not support DS mode\n");
		return -1;
	}

	// Switch to DS mode
	err = sunxi_mmc_switch(sdhci, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, EXT_CSD_TIMING_BC);

	if (err) {
		printk_warning("SMHC: Failed to change to DS mode\n");
		return err;
	}

	// Update speed mode to SDR12
	mmc->speed_mode = MMC_DS26_SDR12;

	return 0;
}

/**
 * @brief Switch the Sunxi SDHCI controller to High Speed (HS) mode.
 * 
 * This function switches the Secure Digital Host Controller Interface (SDHCI) in a Sunxi system-on-a-chip (SoC) environment to High Speed (HS) mode.
 * 
 * @param sdhci A pointer to the Sunxi SDHCI controller structure.
 * @return Returns 0 on success, or a negative error code on failure.
 */
static int sunxi_mmc_mmc_switch_hs(sunxi_sdhci_t *sdhci) {
	mmc_t *mmc = sdhci->mmc;
	int err;

	// Check if already in SDR25 mode
	if (mmc->speed_mode == MMC_HSSDR52_SDR25) {
		printk_trace("SMHC: set in SDR25 mode\n");
	}

	// Check if card supports HS mode
	if (!(mmc->card_caps && MMC_MODE_HS_52MHz)) {
		printk_warning("SMHC: Card does not support HS mode\n");
		return -1;
	}

	// Switch to HS mode
	err = sunxi_mmc_switch(sdhci, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS);

	if (err) {
		printk_warning("SMHC: Failed to change to HS mode\n");
		return err;
	}

	// Update speed mode to SDR25
	mmc->speed_mode = MMC_HSSDR52_SDR25;

	return 0;
}

/**
 * @brief Switch the Sunxi SDHCI controller to High Speed 200 (HS200) mode.
 * 
 * This function switches the Secure Digital Host Controller Interface (SDHCI) in a Sunxi system-on-a-chip (SoC) environment to High Speed 200 (HS200) mode.
 * 
 * @param sdhci A pointer to the Sunxi SDHCI controller structure.
 * @return Returns 0 on success, or a negative error code on failure.
 */
static int sunxi_mmc_mmc_switch_hs200(sunxi_sdhci_t *sdhci) {
	mmc_t *mmc = sdhci->mmc;
	int err;

	// Check if already in SDR104 mode
	if (mmc->speed_mode == MMC_HS200_SDR104) {
		printk_trace("SMHC: set in SDR104 mode\n");
	}

	// Check if card supports HS200 mode
	if (!(mmc->card_caps && MMC_MODE_HS200)) {
		printk_warning("SMHC: Card does not support HS200 mode\n");
		return -1;
	}

	// Switch to HS200 mode
	err = sunxi_mmc_switch(sdhci, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS200);

	if (err) {
		printk_warning("SMHC: Failed to change to HS200 mode\n");
		return err;
	}

	// Update speed mode to SDR104
	mmc->speed_mode = MMC_HS200_SDR104;

	return 0;
}

/**
 * @brief Switch the Sunxi SDHCI controller to High Speed 400 (HS400) mode.
 * 
 * This function switches the Secure Digital Host Controller Interface (SDHCI) in a Sunxi system-on-a-chip (SoC) environment to High Speed 400 (HS400) mode.
 * 
 * @param sdhci A pointer to the Sunxi SDHCI controller structure.
 * @return Returns 0 on success, or a negative error code on failure.
 */
static int sunxi_mmc_mmc_switch_hs400(sunxi_sdhci_t *sdhci) {
	mmc_t *mmc = sdhci->mmc;
	int err;

	// Check if already in HS400 mode
	if (mmc->speed_mode == MMC_HS400) {
		printk_trace("SMHC: set in HS400 mode\n");
	}

	// Check if card supports HS400 mode
	if (!(mmc->card_caps && MMC_MODE_HS400)) {
		printk_warning("SMHC: Card does not support HS400 mode\n");
		return -1;
	}

	// Switch to HS400 mode
	err = sunxi_mmc_switch(sdhci, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS400);

	if (err) {
		printk_warning("SMHC: Failed to change to HS400 mode\n");
		return err;
	}

	// Update speed mode to HS400
	mmc->speed_mode = MMC_HS400;

	return 0;
}

/**
 * @brief Switches the speed mode of the MMC controller.
 *
 * This function switches the speed mode of the MMC controller based on the provided speed mode.
 *
 * @param sdhci Pointer to the SD/MMC host controller structure.
 * @param spd_mode Speed mode to be switched to.
 * @return Returns 0 upon success, -1 if an error occurs.
 */
static int sunxi_mmc_mmc_switch_speed_mode(sunxi_sdhci_t *sdhci, uint32_t spd_mode) {
	mmc_t *mmc = sdhci->mmc;
	int ret = 0;

	if (sunxi_mmc_host_is_spi(mmc)) {
		return 0;
	}

	switch (spd_mode) {
		case MMC_DS26_SDR12:
			ret = sunxi_mmc_mmc_switch_ds(sdhci);
			break;
		case MMC_HSSDR52_SDR25:
			ret = sunxi_mmc_mmc_switch_hs(sdhci);
			break;
		case MMC_HS200_SDR104:
			ret = sunxi_mmc_mmc_switch_hs200(sdhci);
			break;
		case MMC_HS400:
			ret = sunxi_mmc_mmc_switch_hs400(sdhci);
			break;
		default:
			ret = -1;
			printk_debug("SMHC: error speed mode %d\n", spd_mode);
			break;
	}
	return ret;
}

/**
 * @brief Checks if the specified bus width is supported by the MMC controller.
 *
 * This function checks if the specified bus width is supported by the MMC controller, based on the provided parameters.
 *
 * @param sdhci Pointer to the SD/MMC host controller structure.
 * @param emmc_hs_ddr Flag indicating if eMMC high-speed DDR mode is enabled.
 * @param bus_width The bus width to be checked.
 * @return Returns 0 if the bus width is supported, -1 if not.
 */
static int sunxi_mmc_check_bus_width(sunxi_sdhci_t *sdhci, uint32_t emmc_hs_ddr, uint32_t bus_width) {
	mmc_t *mmc = sdhci->mmc;
	int ret = 0;

	if (bus_width == SMHC_WIDTH_1BIT) {
		/* don't consider SD3.0. tSD/fSD is SD2.0, 1-bit can be support */

		if ((emmc_hs_ddr && (!sunxi_mmc_device_is_sd(mmc)) && (mmc->speed_mode == MMC_HSSDR52_SDR25)) ||
			((!sunxi_mmc_device_is_sd(mmc)) && (mmc->speed_mode == MMC_HSDDR52_DDR50)) || ((!sunxi_mmc_device_is_sd(mmc)) && (mmc->speed_mode == MMC_HS200_SDR104)) ||
			((!sunxi_mmc_device_is_sd(mmc)) && (mmc->speed_mode == MMC_HS400))) {
			ret = -1;
		}
	} else if (bus_width == SMHC_WIDTH_1BIT) {
		if (!(mmc->card_caps & MMC_MODE_4BIT)) {
			ret = -1;
		}
	} else if (bus_width == SMHC_WIDTH_8BIT) {
		if (!(mmc->card_caps & MMC_MODE_8BIT))
			ret = -1;
		if (sunxi_mmc_device_is_sd(mmc))
			ret = -1;
	} else {
		printk_debug("SMHC: bus width error %d\n", bus_width);
		ret = -1;
	}

	return ret;
}

/**
 * @brief Switches the bus width of the MMC controller.
 *
 * This function switches the bus width of the MMC controller based on the provided parameters.
 *
 * @param sdhci Pointer to the SD/MMC host controller structure.
 * @param spd_mode Speed mode of the MMC controller.
 * @param width The bus width to be set.
 * @return Returns 0 upon success, -1 if an error occurs.
 */
static int sunxi_mmc_mmc_switch_bus_width(sunxi_sdhci_t *sdhci, uint32_t spd_mode, uint32_t width) {
	mmc_t *mmc = sdhci->mmc;
	int err = 0;
	uint32_t emmc_hs_ddr = 0;
	uint32_t val = 0;

	if (spd_mode == MMC_HS400) {
		return 0;
	}

	if (spd_mode == MMC_DS26_SDR12) {
		emmc_hs_ddr = 1;
	}

	err = sunxi_mmc_check_bus_width(sdhci, emmc_hs_ddr, width);

	if (err) {
		printk_warning("SMHC: bus witdh param error.\n");
		return -1;
	}

	if (width == SMHC_WIDTH_1BIT)
		val = EXT_CSD_BUS_WIDTH_1;
	else if (spd_mode == MMC_HSDDR52_DDR50) {
		if (width == SMHC_WIDTH_4BIT)
			val = EXT_CSD_BUS_DDR_4;
		else if (width == SMHC_WIDTH_8BIT)
			val = EXT_CSD_BUS_DDR_8;
	} else if (width == SMHC_WIDTH_4BIT)
		val = EXT_CSD_BUS_WIDTH_4;
	else if (width == SMHC_WIDTH_8BIT)
		val = EXT_CSD_BUS_WIDTH_8;
	else
		val = EXT_CSD_BUS_WIDTH_1;

	err = sunxi_mmc_switch(sdhci, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BUS_WIDTH, val);

	if (err) {
		printk_warning("SMHC: set bus witdh error.\n");
		return -1;
	}
	if (spd_mode == MMC_HSDDR52_DDR50) {
		mmc->speed_mode = MMC_HSDDR52_DDR50;
	}

	sunxi_mmc_set_bus_width(sdhci, width);

	return err;
}

/**
 * @brief Switches the speed mode and bus width of the MMC controller.
 *
 * This function switches the speed mode and bus width of the MMC controller based on the provided parameters.
 * If the device is an SD card, no action is taken and the function returns successfully.
 *
 * @param sdhci Pointer to the SD/MMC host controller structure.
 * @param spd_mode Speed mode to be switched to.
 * @param width The bus width to be set.
 * @return Returns 0 upon success, an error code if an error occurs.
 */
static int sunxi_mmc_mmc_switch_bus_mode(sunxi_sdhci_t *sdhci, uint32_t spd_mode, uint32_t width) {
	mmc_t *mmc = sdhci->mmc;
	int err = 0;
	int spd_mode_backup = 0;

	if (sunxi_mmc_device_is_sd(mmc)) {
		return 0;
	}

	if (spd_mode == MMC_HSDDR52_DDR50) {
		spd_mode_backup = MMC_HSSDR52_SDR25;
	} else {
		spd_mode_backup = spd_mode;
	}

	err = sunxi_mmc_mmc_switch_speed_mode(sdhci, spd_mode);

	if (err) {
		printk_warning("SMHC: Switch speed mode failed\n");
		return err;
	}

	err = sunxi_mmc_mmc_switch_bus_width(sdhci, spd_mode, width);

	if (err) {
		printk_warning("SMHC: Switch bus width\n");
		return err;
	}

	if (spd_mode == MMC_HSDDR52_DDR50) {
		mmc->speed_mode = MMC_HSDDR52_DDR50;
	}

	return err;
}

/**
 * @brief Sends the SD CMD8 (SEND_IF_COND) command to the MMC controller.
 *
 * This function sends the SD CMD8 command to the MMC controller to check if the card supports the given voltage range.
 *
 * @param sdhci Pointer to the SD/MMC host controller structure.
 * @return Returns 0 upon success, an error code if an error occurs.
 */
static int sunxi_mmc_sd_send_if_cond(sunxi_sdhci_t *sdhci) {
	mmc_t *mmc = sdhci->mmc;
	mmc_cmd_t cmd;
	int err = 0;

	cmd.cmdidx = SD_CMD_SEND_IF_COND;
	/* We set the bit if the host supports voltages between 2.7 and 3.6 V */
	cmd.cmdarg = ((mmc->voltages & 0xff8000) != 0) << 8 | 0xaa;
	cmd.resp_type = MMC_RSP_R7;
	cmd.flags = 0;

	err = sunxi_sdhci_xfer(sdhci, &cmd, NULL);

	if (err) {
		printk_warning("SMHC: send if cond failed\n");
		return err;
	}

	if ((cmd.response[0] & 0xff) != 0xaa)
		return UNUSABLE_ERR;
	else
		mmc->version = SD_VERSION_2;

	return 0;
}

/**
 * @brief Displays information about the SD/MMC card attached to the given host controller.
 *
 * This function prints various details about the SD/MMC card attached to the specified host controller,
 * such as the card type, capacity, CID, CSD, maximum transfer speed, manufacturer ID, OEM/application ID,
 * product name, product revision, serial number, and manufacturing date.
 *
 * @param sdhci Pointer to the SD/MMC host controller structure.
 */
static void sunxi_mmc_show_card_info(sunxi_sdhci_t *sdhci) {
	mmc_t *mmc = sdhci->mmc;
	if (mmc->high_capacity)
		printk_debug("  High capacity card\n");
	printk_debug("  CID: %08X-%08X-%08X-%08X\n", mmc->cid[0], mmc->cid[1], mmc->cid[2], mmc->cid[3]);
	printk_debug("  CSD: %08X-%08X-%08X-%08X\n", mmc->csd[0], mmc->csd[1], mmc->csd[2], mmc->csd[3]);
	printk_debug("  Max transfer speed: %u HZ\n", mmc->tran_speed);
	printk_debug("  SMHC CLK: %uHz\n", mmc->clock);
	printk_debug("  Manufacturer ID: %02X\n", extract_mid(mmc));
	printk_debug("  OEM/Application ID: %04X\n", extract_oid(mmc));
	printk_debug("  Product name: '%c%c%c%c%c'\n", mmc->cid[0] & 0xff, (mmc->cid[1] >> 24), (mmc->cid[1] >> 16) & 0xff, (mmc->cid[1] >> 8) & 0xff, mmc->cid[1] & 0xff);
	printk_debug("  Product revision: %u.%u\n", extract_prv(mmc) >> 4, extract_prv(mmc) & 0xf);
	printk_debug("  Serial no: %0u\n", extract_psn(mmc));
	printk_debug("  Manufacturing date: %u.%u\n", extract_year(mmc), extract_month(mmc));
}

/**
 * @brief Probes the SD/MMC card attached to the given host controller.
 *
 * This function probes the SD/MMC card attached to the specified host controller,
 * retrieves various card-specific data such as CID and CSD, and sets the card
 * to the appropriate mode. It also handles sending commands and checking for errors.
 *
 * @param sdhci Pointer to the SD/MMC host controller structure.
 * @return 0 on success, or an error code if an error occurred during probing.
 */
static int sunxi_mmc_probe(sunxi_sdhci_t *sdhci) {
	mmc_t *mmc = sdhci->mmc;
	int err = 0;
	int timeout = 1000;
	uint64_t capacity = 0, cmult = 0, csize = 0;
	const char *strver = "unknown";

	mmc_cmd_t cmd;
	char ext_csd[512];

	/* Set card to identity Mode */
	cmd.cmdidx = sunxi_mmc_host_is_spi(mmc) ? MMC_CMD_SEND_CID : MMC_CMD_ALL_SEND_CID;
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg = 0;
	cmd.flags = 0;

	err = sunxi_sdhci_xfer(sdhci, &cmd, NULL);

	if (err) {
		printk_warning("SMHC: Put the Card in Identify Mode failed\n");
		return -1;
	}

	memcpy(mmc->cid, cmd.response, 16);

	/*
	 * For MMC cards, set the Relative Address.
	 * For SD cards, get the Relatvie Address.
	 * This also puts the cards into Standby State
	 */
	if (!sunxi_mmc_host_is_spi(mmc)) {
		cmd.cmdidx = SD_CMD_SEND_RELATIVE_ADDR;
		cmd.cmdarg = mmc->rca << 16;
		cmd.resp_type = MMC_RSP_R6;
		cmd.flags = 0;

		err = sunxi_sdhci_xfer(sdhci, &cmd, NULL);

		if (err) {
			printk_warning("SMHC: send rca failed\n");
			return err;
		}

		if (sunxi_mmc_device_is_sd(mmc)) {
			mmc->rca = (cmd.response[0] >> 16) & 0xffff;
		}
	}

	/* Get the Card-Specific Data */
	cmd.cmdidx = MMC_CMD_SEND_CSD;
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg = mmc->rca << 16;
	cmd.flags = 0;

	err = sunxi_sdhci_xfer(sdhci, &cmd, NULL);

	/* Waiting for the ready status */
	sunxi_mmc_send_status(sdhci, timeout);

	if (err) {
		printk_warning("SMHC: MMC get csd failed\n");
		return err;
	}

	mmc->csd[0] = cmd.response[0];
	mmc->csd[1] = cmd.response[1];
	mmc->csd[2] = cmd.response[2];
	mmc->csd[3] = cmd.response[3];

	if (mmc->version == MMC_VERSION_UNKNOWN) {
		uint32_t version = (cmd.response[0] >> 26) & 0xf;
		switch (version) {
			case 0:
				mmc->version = MMC_VERSION_1_2;
				strver = "1.2";
				break;
			case 1:
				mmc->version = MMC_VERSION_1_4;
				strver = "1.4";
				break;
			case 2:
				mmc->version = MMC_VERSION_2_2;
				strver = "2.2";
				break;
			case 3:
				mmc->version = MMC_VERSION_3;
				strver = "3.0";
				break;
			case 4:
				mmc->version = MMC_VERSION_4;
				strver = "4.0";
				break;
			default:
				mmc->version = MMC_VERSION_1_2;
				strver = "1.2";
				break;
		}
	}

	/* divide frequency by 10, since the mults are 10x bigger */
	uint32_t freq = tran_speed_unit[(cmd.response[0] & 0x7)];
	uint32_t mult = tran_speed_time[((cmd.response[0] >> 3) & 0xf)];

	printk_trace("SMHC: Card frep:%uHz, mult:%u\n", freq, mult);

	mmc->tran_speed = freq * mult;
	mmc->read_bl_len = 1 << ((cmd.response[1] >> 16) & 0xf);

	if (sunxi_mmc_device_is_sd(mmc))
		mmc->write_bl_len = mmc->read_bl_len;
	else
		mmc->write_bl_len = 1 << ((cmd.response[3] >> 22) & 0xf);

	if (mmc->high_capacity) {
		csize = (mmc->csd[1] & 0x3f) << 16 | (mmc->csd[2] & 0xffff0000) >> 16;
		cmult = 8;
	} else {
		csize = (mmc->csd[1] & 0x3ff) << 2 | (mmc->csd[2] & 0xc0000000) >> 30;
		cmult = (mmc->csd[2] & 0x00038000) >> 15;
	}

	mmc->capacity = (csize + 1) << (cmult + 2);
	mmc->capacity *= mmc->read_bl_len;

	if (mmc->read_bl_len > 512)
		mmc->read_bl_len = 512;

	if (mmc->write_bl_len > 512)
		mmc->write_bl_len = 512;

	/* Select the card, and put it into Transfer Mode */
	if (!sunxi_mmc_host_is_spi(mmc)) { /* cmd not supported in spi */
		cmd.cmdidx = MMC_CMD_SELECT_CARD;
		cmd.resp_type = MMC_RSP_R1b;
		cmd.cmdarg = mmc->rca << 16;
		cmd.flags = 0;
		err = sunxi_sdhci_xfer(sdhci, &cmd, NULL);

		if (err) {
			printk_warning("SMHC: Select the card failed\n");
			return err;
		}
	}

	sunxi_mmc_set_clock(sdhci, 25000000);

	/*
    * For SD, its erase group is always one sector
    */
	mmc->erase_grp_size = 1;
	mmc->part_config = MMCPART_NOAVAILABLE;

	if (!sunxi_mmc_device_is_sd(mmc) && (mmc->version >= MMC_VERSION_4)) {
		/* check  ext_csd version and capacity */
		err = sunxi_mmc_send_ext_csd(sdhci, ext_csd);
		if (!err) {
			/* update mmc version */
			switch (ext_csd[EXT_CSD_REV]) {
				case 0:
					mmc->version = MMC_VERSION_4;
					strver = "4.0";
					break;
				case 1:
					mmc->version = MMC_VERSION_4_1;
					strver = "4.1";
					break;
				case 2:
					mmc->version = MMC_VERSION_4_2;
					strver = "4.2";
					break;
				case 3:
					mmc->version = MMC_VERSION_4_3;
					strver = "4.3";
					break;
				case 5:
					mmc->version = MMC_VERSION_4_41;
					strver = "4.41";
					break;
				case 6:
					mmc->version = MMC_VERSION_4_5;
					strver = "4.5";
					break;
				case 7:
					mmc->version = MMC_VERSION_5_0;
					strver = "5.0";
					break;
				case 8:
					mmc->version = MMC_VERSION_5_1;
					strver = "5.1";
					break;
			}
		} else {
			printk_debug("SMHC: Read ext csd fail\n");
		}

		if (!err & (ext_csd[EXT_CSD_REV] >= 2)) {
			capacity = ext_csd[EXT_CSD_SEC_CNT] << 0 | ext_csd[EXT_CSD_SEC_CNT + 1] << 8 | ext_csd[EXT_CSD_SEC_CNT + 2] << 16 | ext_csd[EXT_CSD_SEC_CNT + 3] << 24;
			capacity *= mmc->read_bl_len;
			if ((capacity >> 20) > 2 * 1024)
				mmc->capacity = capacity;
		}

		/*
        * Check whether GROUP_DEF is set, if yes, read out
        * group size from ext_csd directly, or calculate
        * the group size from the csd value.
        */
		if (ext_csd[EXT_CSD_ERASE_GROUP_DEF])
			mmc->erase_grp_size = ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] * 512 * 1024;
		else {
			int erase_gsz, erase_gmul;
			erase_gsz = (mmc->csd[2] & 0x00007c00) >> 10;
			erase_gmul = (mmc->csd[2] & 0x000003e0) >> 5;
			mmc->erase_grp_size = (erase_gsz + 1) * (erase_gmul + 1);
		}

		/* store the partition info of emmc */
		if (ext_csd[EXT_CSD_PARTITION_SUPPORT] & PART_SUPPORT) {
			mmc->part_config = ext_csd[EXT_CSD_PART_CONFIG];
		}
	}

	if (sunxi_mmc_device_is_sd(mmc)) {
		err = sunxi_mmc_sd_change_freq(sdhci);
	} else {
		err = sunxi_mmc_mmc_change_freq(sdhci);
	}

	if (err) {
		printk_warning("SMHC: Change speed mode failed\n");
		return err;
	}

	/* for re-update sample phase */
	err = sunxi_sdhci_update_phase(sdhci);
	if (err) {
		printk_warning("SMHC: update clock failed\n");
		return err;
	}

	printk_trace("SMHC: mmc->card_caps 0x%x, ddr caps:0x%x\n", mmc->card_caps, mmc->card_caps & MMC_MODE_DDR_52MHz);

	/* Restrict card's capabilities by what the host can do */
	mmc->card_caps &= mmc->host_caps;
	printk_trace("SMHC: mmc->card_caps 0x%x, ddr caps:0x%x\n", mmc->card_caps, mmc->card_caps & MMC_MODE_DDR_52MHz);

	if (!(mmc->card_caps & MMC_MODE_DDR_52MHz) && !sunxi_mmc_device_is_sd(mmc)) {
		if (mmc->speed_mode == MMC_HSDDR52_DDR50)
			mmc->speed_mode = MMC_HSSDR52_SDR25;
		else
			mmc->speed_mode = MMC_DS26_SDR12;
	}

	if (sunxi_mmc_device_is_sd(mmc)) {
		/* SD CARD */
		if (mmc->card_caps & MMC_MODE_4BIT) {
			cmd.cmdidx = MMC_CMD_APP_CMD;
			cmd.resp_type = MMC_RSP_R1;
			cmd.cmdarg = mmc->rca << 16;
			cmd.flags = 0;

			err = sunxi_sdhci_xfer(sdhci, &cmd, NULL);
			if (err) {
				printk_trace("SMHC: send app cmd failed\n");
				return err;
			}

			cmd.cmdidx = SD_CMD_APP_SET_BUS_WIDTH;
			cmd.resp_type = MMC_RSP_R1;
			cmd.cmdarg = 2;
			cmd.flags = 0;
			err = sunxi_sdhci_xfer(sdhci, &cmd, NULL);
			if (err) {
				printk_trace("SMHC: sd set bus width failed\n");
				return err;
			}
			sunxi_mmc_set_bus_width(sdhci, SMHC_WIDTH_4BIT);
		}

		if (mmc->card_caps & MMC_MODE_HS)
			mmc->tran_speed = 50000000;
		else
			mmc->tran_speed = 25000000;
	} else {
		/* EMMC */
		if (mmc->card_caps & MMC_MODE_8BIT) {
			printk_trace("SMHC: set mmc bus width 8: %s\n", (mmc->card_caps & MMC_MODE_DDR_52MHz) ? "DDR" : "SDR");
			/* Set the card to use 8 bit */
			if ((mmc->card_caps & MMC_MODE_DDR_52MHz)) {
				/* Set the card to use 8 bit ddr */
				err = sunxi_mmc_switch(sdhci, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BUS_WIDTH, EXT_CSD_BUS_DDR_8);
				if (err) {
					printk_warning("SMHC: switch bus width failed\n");
					return err;
				}
				sunxi_mmc_set_bus_width(sdhci, SMHC_WIDTH_8BIT);
			} else {
				/* Set the card to use 8 bit */
				err = sunxi_mmc_switch(sdhci, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BUS_WIDTH, EXT_CSD_BUS_WIDTH_8);
				if (err) {
					printk_warning("SMHC: switch bus width failed\n");
					return err;
				}
				sunxi_mmc_set_bus_width(sdhci, SMHC_WIDTH_8BIT);
			}
		} else if (mmc->card_caps & MMC_MODE_4BIT) {
			printk_trace("SMHC: set mmc bus width 4: %s\n", (mmc->card_caps & MMC_MODE_DDR_52MHz) ? "DDR" : "SDR");
			if ((mmc->card_caps & MMC_MODE_DDR_52MHz)) {
				/* Set the card to use 4 bit ddr */
				err = sunxi_mmc_switch(sdhci, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BUS_WIDTH, EXT_CSD_BUS_DDR_4);
				if (err) {
					printk_warning("SMHC: switch bus width failed\n");
					return err;
				}
				sunxi_mmc_set_bus_width(sdhci, SMHC_WIDTH_4BIT);
			} else {
				/* Set the card to use 4 bit */
				err = sunxi_mmc_switch(sdhci, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BUS_WIDTH, EXT_CSD_BUS_WIDTH_4);
				if (err) {
					printk_warning("SMHC: switch bus width failed\n");
					return err;
				}
				sunxi_mmc_set_bus_width(sdhci, SMHC_WIDTH_4BIT);
			}
		}

		if (mmc->card_caps & MMC_MODE_HS400) {
			printk_trace("SMHC: speed set to max clk: %dHz\n", sdhci->max_clk);
			mmc->tran_speed = sdhci->max_clk;
		}

		if (mmc->card_caps & MMC_MODE_DDR_52MHz) {
			mmc->tran_speed = 52000000;
		} else if (mmc->card_caps & MMC_MODE_HS) {
			if (mmc->card_caps & MMC_MODE_HS_52MHz) {
				mmc->tran_speed = 52000000;
			} else {
				mmc->tran_speed = 26000000;
			}
		} else {
			mmc->tran_speed = 26000000;
		}
	}

	printk_trace("SMHC: set clock to %u\n", mmc->tran_speed);
	sunxi_mmc_set_clock(sdhci, mmc->tran_speed);

	/* fill in device description */
	mmc->blksz = mmc->read_bl_len;
	mmc->lba = mmc->capacity >> 9;

	printk_debug("SD/MMC card at the '%s' host controller:\r\n", sdhci->name);
	printk_debug("  Attached is a %s%s card\r\n", mmc->version & SD_VERSION_SD ? "SD" : "MMC", mmc->version & SD_VERSION_SD ? "" : strver);
	printk_info("  Capacity: %.2fGB\n", (f64) (mmc->lba >> 11) / (f64) 1024.f);
	sunxi_mmc_show_card_info(sdhci);

	return 0;
}

/**
 * @brief Initializes the SD/MMC host controller and attached card.
 *
 * This function initializes the specified SD/MMC host controller and the
 * attached SD/MMC card. It initializes the host controller core, sets the
 * bus width and clock speed, resets the card, and initializes the card
 * based on its type (SD or eMMC). Finally, it probes the card to retrieve
 * card-specific data.
 *
 * @param sdhci_hdl Pointer to the SD/MMC host controller structure.
 * @return 0 on success, or an error code if an error occurred during initialization.
 */
int sunxi_mmc_init(void *sdhci_hdl) {
	sunxi_sdhci_t *sdhci = (sunxi_sdhci_t *) sdhci_hdl;

	mmc_t *mmc = sdhci->mmc;
	int err = 0;

	printk_trace("SMHC: init mmc device\n");

	err = sunxi_sdhci_core_init(sdhci);
	if (err) {
		printk_warning("SMHC: host init failed\n");
		return err;
	}

	sunxi_mmc_set_bus_width(sdhci, SMHC_WIDTH_1BIT);
	sunxi_mmc_set_clock(sdhci, 400000);

	err = sunxi_mmc_go_idle(sdhci);

	if (err) {
		printk_warning("SMHC: Reset card fail\n");
		return err;
	}

	mmc->part_num = 0;

	if (sdhci->sdhci_mmc_type == MMC_TYPE_SD) {
		/* if is SDHCI0 in PF port try SD Card CD pin */
		if (sdhci->pinctrl.gpio_cd.pin != 0) {
			if (sdhci->id == 0 && sunxi_gpio_read(sdhci->pinctrl.gpio_cd.pin) != GPIO_LEVEL_LOW) {
				printk_warning("SMHC: SD Card Get CD error %d\n", sunxi_gpio_read(sdhci->pinctrl.gpio_cd.pin));
				err = -1;
				return err;
			}
		}

		printk_debug("SMHC: Try to init SD Card\n");
		err = sunxi_mmc_sd_send_if_cond(sdhci);
		if (err) {
			printk_warning("SMHC%d: SD Card did not respond to voltage select\n", sdhci->id);
			return -1;
		}
		err = sunxi_mmc_sd_send_op_cond(sdhci);
		if (err) {
			printk_warning("SMHC%d: SD Card did not respond to voltage select\n", sdhci->id);
			return -1;
		}
	} else if (sdhci->sdhci_mmc_type == MMC_TYPE_EMMC) {
		printk_debug("SMHC: Try to init eMMC Card\n");
		err = sunxi_mmc_mmc_send_op_cond(sdhci);
		if (err) {
			printk_warning("SMHC%d: MMC did not respond to voltage select\n", sdhci->id);
			return -1;
		}
	}

	err = sunxi_mmc_probe(sdhci);
	if (err) {
		printk_warning("SMHC%d: SD/MMC Probe failed, err %d\n", sdhci->id, err);
	}

	return err;
}

/**
 * @brief Read blocks from the Sunxi MMC block device
 *
 * This function reads a specified number of blocks from the Sunxi MMC block device
 * and stores the data into the destination buffer.
 *
 * @param sdhci     Pointer to the Sunxi SD Host Controller instance
 * @param dst       Pointer to the destination buffer where the read data will be stored
 * @param start     The starting block number to read from
 * @param blkcnt    The number of blocks to read
 *
 * @return          Returns 0 on success, or an error code if the operation fails
 */
uint32_t sunxi_mmc_blk_read(void *sdhci, void *dst, uint32_t start, uint32_t blkcnt) {
	return sunxi_mmc_read_blocks((sunxi_sdhci_t *) sdhci, dst, start, blkcnt);
}

/**
 * @brief Writes blocks of data to the MMC device using the specified SDHCI instance.
 *
 * This function writes a specified number of blocks to the MMC (MultiMediaCard) device
 * associated with the given SDHCI (SD Host Controller Interface) instance. It serves as a
 * wrapper around the sunxi_mmc_write_blocks function, providing a simplified interface for
 * block data write operations.
 *
 * @param[in] sdhci A pointer to the SDHCI instance.
 * @param[in] dst A pointer to the destination buffer from which data will be written to the MMC.
 * @param[in] start The starting block number where the data writing begins.
 * @param[in] blkcnt The number of blocks to write.
 *
 * @return The number of blocks successfully written, or 0 if writing failed.
 */
uint32_t sunxi_mmc_blk_write(void *sdhci, void *dst, uint32_t start, uint32_t blkcnt) {
	return sunxi_mmc_write_blocks((sunxi_sdhci_t *) sdhci, dst, start, blkcnt);
}
