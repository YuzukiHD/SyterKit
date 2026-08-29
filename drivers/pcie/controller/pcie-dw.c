/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "pcie-dw: " fmt

/**
 * @file pcie-dw.c
 * @brief DesignWare PCIe controller driver.
 *
 * Implements the DBI and application register access paths, ATU window
 * programming, link training, and the generic controller API dispatcher used
 * by the root-complex and endpoint drivers.
 */

#include <stdbool.h>
#include <stdint.h>

#include <io.h>
#include <log.h>
#include <timer.h>

#include <drivers/pcie/controller/pcie-controller.h>

#define PCIE_DW_DBI2_BASE               0x100000U
#define PCIE_DW_DBI2_BAR_ENABLE         0x1U

/**
 * @brief Check that a configuration access width is supported.
 *
 * @param[in] size Access width in bytes.
 * @return true when @p size is 1, 2, or 4.
 */
static bool pcie_access_size_valid(uint8_t size)
{
	return size == 1U || size == 2U || size == 4U;
}

/**
 * @brief Validate a DBI register access.
 *
 * @param[in] controller PCIe controller descriptor.
 * @param[in] offset Register offset.
 * @param[in] size Access width in bytes.
 * @return PCIE_OK when the access is in range and aligned.
 */
static int pcie_dbi_access_valid(const struct pcie_controller *controller,
		uint32_t offset, uint8_t size)
{
	if (controller == NULL || !pcie_access_size_valid(size) ||
	    (offset & ((uint32_t)size - 1U)) != 0U || controller->config.dbi_base == 0U)
		return PCIE_ERR_INVALID;
	if (controller->config.dbi_size != 0U &&
	    (uint64_t)offset + size > controller->config.dbi_size)
		return PCIE_ERR_INVALID;
	return PCIE_OK;
}

/**
 * @brief Validate an application register access.
 *
 * @param[in] controller PCIe controller descriptor.
 * @param[in] offset Register offset.
 * @return PCIE_OK when the access is in range.
 */
static int pcie_app_access_valid(const struct pcie_controller *controller,
		uint32_t offset)
{
	if (controller == NULL || controller->config.app_base == 0U)
		return PCIE_ERR_INVALID;
	if (controller->config.app_size != 0U &&
	    (uint64_t)offset + sizeof(uint32_t) > controller->config.app_size)
		return PCIE_ERR_INVALID;
	return PCIE_OK;
}

/**
 * @brief Read a DBI configuration-space field.
 *
 * @param[in] controller PCIe controller descriptor.
 * @param[in] offset Register offset.
 * @param[in] size Access width in bytes.
 * @param[out] value Receives the read value.
 * @return PCIE_OK on success, otherwise an error code.
 */
static int pcie_dw_read_dbi(struct pcie_controller *controller,
		uint32_t offset, uint8_t size, uint32_t *value)
{
	uintptr_t address;

	if (value == NULL || pcie_dbi_access_valid(controller, offset, size) != PCIE_OK)
		return PCIE_ERR_INVALID;
	address = controller->config.dbi_base + offset;
	if (size == 1U)
		*value = readb(address);
	else if (size == 2U)
		*value = readw(address);
	else
		*value = readl(address);
	return PCIE_OK;
}

/**
 * @brief Write a DBI configuration-space field.
 *
 * @param[in] controller PCIe controller descriptor.
 * @param[in] offset Register offset.
 * @param[in] size Access width in bytes.
 * @param[in] value Value to write.
 * @return PCIE_OK on success, otherwise an error code.
 */
static int pcie_dw_write_dbi(struct pcie_controller *controller,
		uint32_t offset, uint8_t size, uint32_t value)
{
	uintptr_t address;

	if (pcie_dbi_access_valid(controller, offset, size) != PCIE_OK)
		return PCIE_ERR_INVALID;
	address = controller->config.dbi_base + offset;
	if (size == 1U)
		writeb(value, address);
	else if (size == 2U)
		writew(value, address);
	else
		writel(value, address);
	return PCIE_OK;
}

/**
 * @brief Read an application register.
 *
 * @param[in] controller PCIe controller descriptor.
 * @param[in] offset Register offset.
 * @param[out] value Receives the read value.
 * @return PCIE_OK on success, otherwise an error code.
 */
static int pcie_dw_read_app(struct pcie_controller *controller,
		uint32_t offset, uint32_t *value)
{
	if (value == NULL || pcie_app_access_valid(controller, offset) != PCIE_OK)
		return PCIE_ERR_INVALID;
	*value = readl(controller->config.app_base + offset);
	return PCIE_OK;
}

/**
 * @brief Write an application register.
 *
 * @param[in] controller PCIe controller descriptor.
 * @param[in] offset Register offset.
 * @param[in] value Value to write.
 * @return PCIE_OK on success, otherwise an error code.
 */
static int pcie_dw_write_app(struct pcie_controller *controller,
		uint32_t offset, uint32_t value)
{
	if (pcie_app_access_valid(controller, offset) != PCIE_OK)
		return PCIE_ERR_INVALID;
	writel(value, controller->config.app_base + offset);
	return PCIE_OK;
}

/**
 * @brief Validate a DesignWare controller configuration.
 *
 * @param[in] controller PCIe controller descriptor.
 * @return PCIE_OK when the configuration is consistent and in range.
 */
static int pcie_dw_init(struct pcie_controller *controller)
{
	const struct pcie_controller_config *config;
	uintptr_t address_max;

	if (controller == NULL)
		return PCIE_ERR_INVALID;
	config = &controller->config;
	address_max = (uintptr_t)-1;
	if ((config->mode != PCIE_MODE_RC && config->mode != PCIE_MODE_EP) ||
	    config->dbi_base == 0U || config->app_base == 0U ||
	    config->lanes == 0U || config->link_gen == 0U ||
	    (config->mode == PCIE_MODE_EP && config->ep_function_stride == 0U) ||
	    config->max_lanes < config->lanes ||
	    config->max_link_gen < config->link_gen ||
	    config->num_ob_windows > 16U || config->num_ib_windows > 16U ||
	    (config->dbi_size != 0U &&
	     config->dbi_base > address_max - (config->dbi_size - 1U)) ||
	    (config->app_size != 0U &&
	     config->app_base > address_max - (config->app_size - 1U)) ||
	    (config->mode == PCIE_MODE_RC &&
	     (config->cfg_cpu_addr == 0U || config->cfg_size < 0x1000U ||
	      config->cfg_cpu_addr > address_max - (config->cfg_size - 1U))) ||
	    (config->mem_size != 0U &&
	     (config->mem_cpu_addr == 0U ||
	      config->mem_cpu_addr > address_max - (config->mem_size - 1U) ||
	      config->mem_pci_addr > (uint64_t)-1 - (config->mem_size - 1U))) ||
	    (config->io_size != 0U &&
	     (config->io_cpu_addr == 0U ||
	      config->io_cpu_addr > address_max - (config->io_size - 1U) ||
	      config->io_pci_addr > (uint64_t)-1 - (config->io_size - 1U))))
		return PCIE_ERR_INVALID;
	return PCIE_OK;
}

static int pcie_dw_disable_atu(struct pcie_controller *controller,
		enum pcie_atu_direction direction, uint8_t index);

/**
 * @brief Disable all ATU windows and tear down the controller.
 *
 * @param[in,out] controller PCIe controller descriptor.
 */
static void pcie_dw_exit(struct pcie_controller *controller)
{
	uint32_t ob_windows;
	uint32_t ib_windows;
	uint32_t index;

	if (controller == NULL)
		return;
	ob_windows = controller->config.num_ob_windows != 0U ?
		controller->config.num_ob_windows : 8U;
	ib_windows = controller->config.num_ib_windows != 0U ?
		controller->config.num_ib_windows : 8U;
	for (index = 0U; index < ob_windows; ++index)
		(void)pcie_dw_disable_atu(controller, PCIE_ATU_OUTBOUND,
			(uint8_t)index);
	for (index = 0U; index < ib_windows; ++index)
		(void)pcie_dw_disable_atu(controller, PCIE_ATU_INBOUND,
			(uint8_t)index);
}

/**
 * @brief Set the controller operating mode.
 *
 * On sun55iw6 the role is platform-fixed; only the LTSSM enable is driven from
 * the application registers, so this accepts either mode.
 *
 * @param[in] controller PCIe controller descriptor.
 * @param[in] mode PCIE_MODE_RC or PCIE_MODE_EP.
 * @return PCIE_OK on success, PCIE_ERR_INVALID on a bad mode.
 */
static int pcie_dw_set_mode(struct pcie_controller *controller,
		enum pcie_mode mode)
{
	if (controller == NULL ||
	    (mode != PCIE_MODE_RC && mode != PCIE_MODE_EP))
		return PCIE_ERR_INVALID;
	/* sun55iw6 uses APP 0xc00 only for LTSSM enable; role is platform-fixed. */
	return PCIE_OK;
}

/**
 * @brief Enable or disable writes to read-only DBI fields.
 *
 * @param[in,out] controller PCIe controller descriptor.
 * @param[in] enable true to allow read-only field writes.
 * @return PCIE_OK on success, otherwise an error code.
 */
static int pcie_dw_set_dbi_ro_write(struct pcie_controller *controller,
		bool enable)
{
	uint32_t value;
	int ret;

	ret = pcie_dw_read_dbi(controller, PCIE_DW_MISC_CONTROL_1, 4U, &value);
	if (ret)
		return ret;
	if (enable)
		value |= PCIE_DW_DBI_RO_WRITE_ENABLE;
	else
		value &= ~PCIE_DW_DBI_RO_WRITE_ENABLE;
	return pcie_dw_write_dbi(controller, PCIE_DW_MISC_CONTROL_1, 4U, value);
}

/**
 * @brief Enable or disable an endpoint BAR in the DBI2 address space.
 *
 * @param[in,out] controller PCIe controller descriptor.
 * @param[in] function Endpoint function number.
 * @param[in] bar BAR number.
 * @param[in] enable true to enable the BAR, false to disable it.
 * @param[in] bar_64bit true when the BAR is 64-bit wide.
 * @return PCIE_OK on success, otherwise an error code.
 */
static int pcie_dw_set_ep_bar(struct pcie_controller *controller,
		uint8_t function, uint8_t bar, bool enable, bool bar_64bit)
{
	uint32_t offset;
	uint64_t offset64;
	int ret;

	if (controller == NULL || function >= 8U || bar >= 6U ||
	    controller->config.ep_function_stride == 0U ||
	    (bar_64bit && bar != 0U && bar != 4U))
		return PCIE_ERR_INVALID;
	offset64 = (uint64_t)PCIE_DW_DBI2_BASE +
		(uint64_t)function * controller->config.ep_function_stride +
		PCIE_CFG_BAR0 + 4U * bar;
	if (offset64 > 0xffffffffULL)
		return PCIE_ERR_INVALID;
	offset = (uint32_t)offset64;
	ret = pcie_dbi_access_valid(controller, offset, 4U);
	if (ret)
		return ret;
	ret = pcie_dw_write_dbi(controller, offset, 4U,
		enable ? PCIE_DW_DBI2_BAR_ENABLE : 0U);
	if (!ret && !enable && bar_64bit)
		ret = pcie_dw_write_dbi(controller, offset + 4U, 4U, 0U);
	return ret;
}

/**
 * @brief Program the link width, speed, and lane mode.
 *
 * @param[in,out] controller PCIe controller descriptor.
 * @return PCIE_OK on success, otherwise an error code.
 */
static int pcie_dw_set_link(struct pcie_controller *controller)
{
	uint32_t value;
	uint32_t link_cap;
	int capability;
	uint32_t link_width;
	uint32_t port_mode;
	int ret;

	if (controller->config.lanes == 0U || controller->config.link_gen == 0U ||
	    controller->config.link_gen > controller->config.max_link_gen ||
	    controller->config.lanes > controller->config.max_lanes)
		return PCIE_ERR_INVALID;
	switch (controller->config.lanes) {
	case 1U:
	case 2U:
	case 4U:
		break;
	default:
		return PCIE_ERR_UNSUPPORTED;
	}
	capability = pcie_controller_find_capability(controller, 0U,
		PCIE_CAP_ID_EXPRESS);
	if (capability < 0)
		return capability;
	ret = pcie_dw_set_dbi_ro_write(controller, true);
	if (ret)
		return ret;

	ret = pcie_dw_read_dbi(controller, capability + PCIE_CAP_EXP_LINK_CAP,
		4U, &link_cap);
	if (ret)
		goto disable_ro_write;
	ret = pcie_dw_read_dbi(controller, capability + PCIE_CAP_EXP_LINK_CTRL2,
		4U, &value);
	if (ret)
		goto disable_ro_write;
	value &= ~PCIE_CAP_EXP_LINK_SPEED_MASK;
	value |= controller->config.link_gen & PCIE_CAP_EXP_LINK_SPEED_MASK;
	ret = pcie_dw_write_dbi(controller, capability + PCIE_CAP_EXP_LINK_CTRL2,
		4U, value);
	if (ret)
		goto disable_ro_write;
	link_cap &= ~PCIE_CAP_EXP_LINK_SPEED_MASK;
	link_cap |= controller->config.link_gen & PCIE_CAP_EXP_LINK_SPEED_MASK;
	ret = pcie_dw_write_dbi(controller, capability + PCIE_CAP_EXP_LINK_CAP,
		4U, link_cap);
	if (ret)
		goto disable_ro_write;

	ret = pcie_dw_read_dbi(controller, PCIE_DW_PORT_LINK_CONTROL, 4U,
		&port_mode);
	if (ret)
		goto disable_ro_write;
	port_mode &= ~PCIE_DW_PORT_LINK_MODE_MASK;
	if (controller->config.lanes == 1U)
		port_mode |= PCIE_DW_PORT_LINK_MODE_1;
	else if (controller->config.lanes == 2U)
		port_mode |= PCIE_DW_PORT_LINK_MODE_2;
	else if (controller->config.lanes == 4U)
		port_mode |= PCIE_DW_PORT_LINK_MODE_4;
	else {
		ret = PCIE_ERR_UNSUPPORTED;
		goto disable_ro_write;
	}
	ret = pcie_dw_write_dbi(controller, PCIE_DW_PORT_LINK_CONTROL, 4U,
		port_mode);
	if (ret)
		goto disable_ro_write;

	ret = pcie_dw_read_dbi(controller, PCIE_DW_LINK_WIDTH_SPEED_CTRL, 4U,
		&link_width);
	if (ret)
		goto disable_ro_write;
	link_width &= ~PCIE_DW_LINK_WIDTH_MASK;
	if (controller->config.lanes == 1U)
		link_width |= PCIE_DW_LINK_WIDTH_1;
	else if (controller->config.lanes == 2U)
		link_width |= PCIE_DW_LINK_WIDTH_2;
	else if (controller->config.lanes == 4U)
		link_width |= PCIE_DW_LINK_WIDTH_4;
	else
		ret = PCIE_ERR_UNSUPPORTED;
	if (!ret)
		ret = pcie_dw_write_dbi(controller, PCIE_DW_LINK_WIDTH_SPEED_CTRL, 4U,
			link_width);

disable_ro_write:
	if (pcie_dw_set_dbi_ro_write(controller, false) != PCIE_OK)
		return PCIE_ERR_IO;
	return ret;
}

/**
 * @brief Retrain the link to a higher generation.
 *
 * @param[in,out] controller PCIe controller descriptor.
 * @param[in] link_gen Target link generation.
 * @return PCIE_OK on success, otherwise an error code.
 */
static int pcie_dw_change_speed(struct pcie_controller *controller,
		uint8_t link_gen)
{
	uint32_t value;
	int capability;
	uint64_t start;
	uint32_t timeout_us;
	int ret;

	if (controller == NULL || link_gen == 0U ||
	    link_gen > controller->config.max_link_gen)
		return PCIE_ERR_INVALID;
	capability = pcie_controller_find_capability(controller, 0U,
		PCIE_CAP_ID_EXPRESS);
	if (capability < 0)
		return capability;
	ret = pcie_dw_set_dbi_ro_write(controller, true);
	if (ret)
		return ret;
	ret = pcie_dw_read_dbi(controller, capability + PCIE_CAP_EXP_LINK_CTRL2,
		4U, &value);
	if (ret)
		goto disable_ro_write;
	value = (value & ~PCIE_CAP_EXP_LINK_SPEED_MASK) |
		(link_gen & PCIE_CAP_EXP_LINK_SPEED_MASK);
	ret = pcie_dw_write_dbi(controller, capability + PCIE_CAP_EXP_LINK_CTRL2,
		4U, value);
	if (ret)
		goto disable_ro_write;
	ret = pcie_dw_read_dbi(controller, PCIE_DW_LINK_WIDTH_SPEED_CTRL, 4U,
		&value);
	if (ret)
		goto disable_ro_write;
	value &= ~PCIE_DW_SPEED_CHANGE;
	ret = pcie_dw_write_dbi(controller, PCIE_DW_LINK_WIDTH_SPEED_CTRL, 4U,
		value);
	if (ret)
		goto disable_ro_write;
	ret = pcie_dw_read_dbi(controller, PCIE_DW_LINK_WIDTH_SPEED_CTRL, 4U,
		&value);
	if (ret)
		goto disable_ro_write;
	ret = pcie_dw_write_dbi(controller, PCIE_DW_LINK_WIDTH_SPEED_CTRL, 4U,
		value | PCIE_DW_SPEED_CHANGE);
	if (ret)
		goto disable_ro_write;

	timeout_us = controller->config.timeout_us != 0U ?
		controller->config.timeout_us : 1000000U;
	start = time_us();
	for (;;) {
		ret = pcie_dw_read_dbi(controller, PCIE_DW_LINK_WIDTH_SPEED_CTRL,
			4U, &value);
		if (ret)
			goto disable_ro_write;
		if ((value & PCIE_DW_SPEED_CHANGE) == 0U)
			break;
		if (time_us() - start >= timeout_us) {
			ret = PCIE_ERR_TIMEOUT;
			goto disable_ro_write;
		}
		udelay(10U);
	}

disable_ro_write:
	if (pcie_dw_set_dbi_ro_write(controller, false) != PCIE_OK)
		return PCIE_ERR_IO;
	if (!ret)
		controller->config.link_gen = link_gen;
	return ret;
}

/**
 * @brief Enable or disable the link training state machine.
 *
 * @param[in,out] controller PCIe controller descriptor.
 * @param[in] enable true to start training, false to stop it.
 * @return PCIE_OK on success, otherwise an error code.
 */
static int pcie_dw_ltssm(struct pcie_controller *controller, bool enable)
{
	uint32_t value;
	int ret;

	ret = pcie_dw_read_app(controller, PCIE_DW_LTSSM_CTRL, &value);
	if (ret)
		return ret;
	if (enable)
		value |= PCIE_DW_LTSSM_ENABLE;
	else
		value &= ~PCIE_DW_LTSSM_ENABLE;
	return pcie_dw_write_app(controller, PCIE_DW_LTSSM_CTRL, value);
}

/**
 * @brief Report whether the link is up.
 *
 * @param[in] controller PCIe controller descriptor.
 * @return true when the link status registers indicate an up link.
 */
static bool pcie_dw_link_up(struct pcie_controller *controller)
{
	uint32_t value;

	if (pcie_dw_read_app(controller, PCIE_DW_LINK_STATUS, &value) != PCIE_OK)
		return false;
	return (value & PCIE_DW_LINK_UP_MASK) == PCIE_DW_LINK_UP_MASK;
}

/**
 * @brief Wait for the link to come up.
 *
 * @param[in] controller PCIe controller descriptor.
 * @param[in] timeout_us Timeout in microseconds, or zero for the default.
 * @return PCIE_OK when the link is up, PCIE_ERR_TIMEOUT otherwise.
 */
static int pcie_dw_wait_link(struct pcie_controller *controller,
		uint32_t timeout_us)
{
	uint64_t start;

	if (controller == NULL)
		return PCIE_ERR_INVALID;
	if (timeout_us == 0U)
		timeout_us = controller->config.timeout_us != 0U ?
			controller->config.timeout_us : 1000000U;
	start = time_us();
	for (;;) {
		if (pcie_dw_link_up(controller))
			return PCIE_OK;
		if (time_us() - start >= timeout_us)
			return PCIE_ERR_TIMEOUT;
		udelay(100U);
	}
}

/**
 * @brief Compute the register address of an ATU window field.
 *
 * @param[in] controller PCIe controller descriptor.
 * @param[in] region ATU window descriptor.
 * @param[in] offset Field offset within the window.
 * @return The absolute register address.
 */
static uintptr_t pcie_dw_atu_reg(const struct pcie_controller *controller,
		const struct pcie_atu_region *region, uint32_t offset)
{
	uint32_t direction_offset = region->direction == PCIE_ATU_INBOUND ?
		PCIE_DW_ATU_INBOUND_OFFSET : PCIE_DW_ATU_OUTBOUND_OFFSET;

	return controller->config.dbi_base + PCIE_DW_ATU_BASE +
		direction_offset + (uint32_t)region->index * PCIE_DW_ATU_REGION_STRIDE + offset;
}

/**
 * @brief Check that an address range does not wrap.
 *
 * @param[in] address Range base address.
 * @param[in] size Range size in bytes.
 * @param[out] last Receives the last address of the range.
 * @return true when the range is valid.
 */
static bool pcie_dw_u64_range_valid(uint64_t address, uint64_t size,
		uint64_t *last)
{
	if (last == NULL || size == 0U ||
	    size - 1U > (uint64_t)-1 - address)
		return false;
	*last = address + size - 1U;
	return true;
}

/**
 * @brief Program an ATU address translation window.
 *
 * Validates the window and region parameters, computes the base, limit, and
 * target addresses, and programs the ATU control and address registers.
 *
 * @param[in,out] controller PCIe controller descriptor.
 * @param[in] region ATU window descriptor.
 * @return PCIE_OK on success, otherwise an error code.
 */
static int pcie_dw_program_atu(struct pcie_controller *controller,
		const struct pcie_atu_region *region)
{
	uint64_t base;
	uint64_t limit;
	uint64_t target;
	uint64_t target_last;
	uint32_t ctrl1;
	uint32_t ctrl2;
	uint32_t windows;
	uint32_t direction_offset;

	if (controller == NULL || region == NULL ||
	    (region->direction != PCIE_ATU_OUTBOUND &&
	     region->direction != PCIE_ATU_INBOUND) ||
	    (region->type != PCIE_ATU_TYPE_MEM &&
	     region->type != PCIE_ATU_TYPE_IO &&
	     region->type != PCIE_ATU_TYPE_CFG0 &&
	     region->type != PCIE_ATU_TYPE_CFG1))
		return PCIE_ERR_INVALID;
	windows = region->direction == PCIE_ATU_INBOUND ?
		controller->config.num_ib_windows : controller->config.num_ob_windows;
	if (windows == 0U)
		windows = 8U;
	if (region->index >= windows || region->index >= 16U ||
	    region->function >= 8U || region->bar >= 6U)
		return PCIE_ERR_INVALID;
	direction_offset = region->direction == PCIE_ATU_INBOUND ?
		PCIE_DW_ATU_INBOUND_OFFSET : PCIE_DW_ATU_OUTBOUND_OFFSET;
	if (controller->config.dbi_size != 0U &&
	    PCIE_DW_ATU_BASE + direction_offset +
	    (uint32_t)region->index * PCIE_DW_ATU_REGION_STRIDE +
	    PCIE_DW_ATU_LIMIT_HI + sizeof(uint32_t) > controller->config.dbi_size)
		return PCIE_ERR_INVALID;

	if (region->direction == PCIE_ATU_INBOUND && region->bar_match) {
		base = 0U;
		limit = 0U;
		target = region->cpu_addr;
		if (!pcie_dw_u64_range_valid(target, region->size, &target_last))
			return PCIE_ERR_INVALID;
		limit = 0U;
	} else if (region->direction == PCIE_ATU_OUTBOUND) {
		base = region->cpu_addr;
		target = region->pci_addr;
		if (!pcie_dw_u64_range_valid(base, region->size, &limit))
			return PCIE_ERR_INVALID;
		if (!pcie_dw_u64_range_valid(target, region->size, &target_last))
			return PCIE_ERR_INVALID;
	} else {
		base = region->pci_addr;
		target = region->cpu_addr;
		if (!pcie_dw_u64_range_valid(base, region->size, &limit))
			return PCIE_ERR_INVALID;
		if (!pcie_dw_u64_range_valid(target, region->size, &target_last))
			return PCIE_ERR_INVALID;
	}

	if ((base & ((uint64_t)PCIE_DW_ATU_ALIGNMENT - 1U)) != 0U ||
	    (target & ((uint64_t)PCIE_DW_ATU_ALIGNMENT - 1U)) != 0U)
		return PCIE_ERR_INVALID;
	ctrl1 = (uint32_t)region->type;
	if ((limit >> 32) > (base >> 32))
		ctrl1 |= PCIE_DW_ATU_INCREASE_REGION_SIZE;
	ctrl2 = PCIE_DW_ATU_ENABLE;
	if (region->function_match) {
		ctrl1 |= PCIE_DW_ATU_FUNC_NUM(region->function);
		ctrl2 |= PCIE_DW_ATU_FUNC_MATCH;
	}
	if (region->bar_match)
		ctrl2 |= PCIE_DW_ATU_BAR_MODE | ((uint32_t)region->bar << 8);

	/* Disable the region while changing its address tuple. */
	writel(0U, pcie_dw_atu_reg(controller, region, PCIE_DW_ATU_CTRL2));
	writel((uint32_t)base, pcie_dw_atu_reg(controller, region,
		PCIE_DW_ATU_BASE_LO));
	writel((uint32_t)(base >> 32), pcie_dw_atu_reg(controller, region,
		PCIE_DW_ATU_BASE_HI));
	writel((uint32_t)limit, pcie_dw_atu_reg(controller, region,
		PCIE_DW_ATU_LIMIT_LO));
	writel((uint32_t)target, pcie_dw_atu_reg(controller, region,
		PCIE_DW_ATU_TARGET_LO));
	writel((uint32_t)(target >> 32), pcie_dw_atu_reg(controller, region,
		PCIE_DW_ATU_TARGET_HI));
	writel((uint32_t)(limit >> 32), pcie_dw_atu_reg(controller, region,
		PCIE_DW_ATU_LIMIT_HI));
	writel(ctrl1, pcie_dw_atu_reg(controller, region, PCIE_DW_ATU_CTRL1));
	writel(ctrl2, pcie_dw_atu_reg(controller, region, PCIE_DW_ATU_CTRL2));
	return PCIE_OK;
}

/**
 * @brief Disable an ATU address translation window.
 *
 * @param[in] controller PCIe controller descriptor.
 * @param[in] direction PCIE_ATU_OUTBOUND or PCIE_ATU_INBOUND.
 * @param[in] index Window index.
 * @return PCIE_OK on success, otherwise an error code.
 */
static int pcie_dw_disable_atu(struct pcie_controller *controller,
		enum pcie_atu_direction direction, uint8_t index)
{
	uint32_t windows;
	uint32_t direction_offset;
	uint32_t offset;
	uintptr_t address;

	if (controller == NULL ||
	    (direction != PCIE_ATU_OUTBOUND && direction != PCIE_ATU_INBOUND) ||
	    index >= 16U || controller->config.dbi_base == 0U)
		return PCIE_ERR_INVALID;
	windows = direction == PCIE_ATU_INBOUND ? controller->config.num_ib_windows :
		controller->config.num_ob_windows;
	if (windows != 0U && index >= windows)
		return PCIE_ERR_INVALID;
	direction_offset = direction == PCIE_ATU_INBOUND ?
		PCIE_DW_ATU_INBOUND_OFFSET : PCIE_DW_ATU_OUTBOUND_OFFSET;
	offset = PCIE_DW_ATU_BASE + direction_offset +
		(uint32_t)index * PCIE_DW_ATU_REGION_STRIDE + PCIE_DW_ATU_CTRL2;
	if (controller->config.dbi_size != 0U &&
	    (uint64_t)offset + sizeof(uint32_t) > controller->config.dbi_size)
		return PCIE_ERR_INVALID;
	address = controller->config.dbi_base + offset;
	writel(0U, address);
	return PCIE_OK;
}

/** @brief DesignWare PCIe controller operations table. */
static const struct pcie_controller_ops pcie_dw_ops = {
	.init = pcie_dw_init,
	.exit = pcie_dw_exit,
	.read_dbi = pcie_dw_read_dbi,
	.write_dbi = pcie_dw_write_dbi,
	.dbi_ro_write_enable = pcie_dw_set_dbi_ro_write,
	.set_ep_bar = pcie_dw_set_ep_bar,
	.read_app = pcie_dw_read_app,
	.write_app = pcie_dw_write_app,
	.set_mode = pcie_dw_set_mode,
	.set_link = pcie_dw_set_link,
	.change_speed = pcie_dw_change_speed,
	.ltssm = pcie_dw_ltssm,
	.link_up = pcie_dw_link_up,
	.wait_link = pcie_dw_wait_link,
	.program_atu = pcie_dw_program_atu,
	.disable_atu = pcie_dw_disable_atu,
};

/**
 * @brief Initialize a PCIe controller with the DesignWare operations.
 *
 * @param[out] controller Controller descriptor to initialize.
 * @param[in] config Controller configuration to apply.
 * @return PCIE_OK on success, otherwise an error code.
 */
int pcie_controller_init(struct pcie_controller *controller,
		const struct pcie_controller_config *config)
{
	return pcie_controller_init_with_ops(controller, config, &pcie_dw_ops);
}

/**
 * @brief Initialize a PCIe controller with explicit operations.
 *
 * @param[out] controller Controller descriptor to initialize.
 * @param[in] config Controller configuration to apply.
 * @param[in] ops Controller operations table.
 * @return PCIE_OK on success, otherwise an error code.
 */
int pcie_controller_init_with_ops(struct pcie_controller *controller,
		const struct pcie_controller_config *config,
		const struct pcie_controller_ops *ops)
{
	int ret;

	if (controller == NULL || config == NULL || ops == NULL || ops->init == NULL)
		return PCIE_ERR_INVALID;
	*controller = (struct pcie_controller){ 0 };
	controller->config = *config;
	controller->ops = ops;
	ret = controller->ops->init(controller);
	if (ret)
		return ret;
	controller->initialized = true;
	return PCIE_OK;
}

/**
 * @brief Tear down a PCIe controller.
 *
 * @param[in,out] controller Controller descriptor to deinitialize.
 */
void pcie_controller_exit(struct pcie_controller *controller)
{
	if (controller == NULL || !controller->initialized)
		return;
	if (controller->ops != NULL && controller->ops->exit != NULL)
		controller->ops->exit(controller);
	controller->initialized = false;
}

/**
 * @brief Read a DBI configuration-space field through the controller ops.
 *
 * @param[in] controller PCIe controller descriptor.
 * @param[in] offset Register offset.
 * @param[in] size Access width in bytes.
 * @param[out] value Receives the read value.
 * @return PCIE_OK on success, otherwise an error code.
 */
int pcie_controller_dbi_read(struct pcie_controller *controller,
		uint32_t offset, uint8_t size, uint32_t *value)
{
	if (controller == NULL || !controller->initialized || controller->ops == NULL ||
	    controller->ops->read_dbi == NULL)
		return PCIE_ERR_INVALID;
	return controller->ops->read_dbi(controller, offset, size, value);
}

/**
 * @brief Write a DBI configuration-space field through the controller ops.
 *
 * @param[in] controller PCIe controller descriptor.
 * @param[in] offset Register offset.
 * @param[in] size Access width in bytes.
 * @param[in] value Value to write.
 * @return PCIE_OK on success, otherwise an error code.
 */
int pcie_controller_dbi_write(struct pcie_controller *controller,
		uint32_t offset, uint8_t size, uint32_t value)
{
	if (controller == NULL || !controller->initialized || controller->ops == NULL ||
	    controller->ops->write_dbi == NULL)
		return PCIE_ERR_INVALID;
	return controller->ops->write_dbi(controller, offset, size, value);
}

/**
 * @brief Read an application register through the controller ops.
 *
 * @param[in] controller PCIe controller descriptor.
 * @param[in] offset Register offset.
 * @param[out] value Receives the read value.
 * @return PCIE_OK on success, otherwise an error code.
 */
int pcie_controller_app_read(struct pcie_controller *controller,
		uint32_t offset, uint32_t *value)
{
	if (controller == NULL || !controller->initialized || controller->ops == NULL ||
	    controller->ops->read_app == NULL)
		return PCIE_ERR_INVALID;
	return controller->ops->read_app(controller, offset, value);
}

/**
 * @brief Write an application register through the controller ops.
 *
 * @param[in] controller PCIe controller descriptor.
 * @param[in] offset Register offset.
 * @param[in] value Value to write.
 * @return PCIE_OK on success, otherwise an error code.
 */
int pcie_controller_app_write(struct pcie_controller *controller,
		uint32_t offset, uint32_t value)
{
	if (controller == NULL || !controller->initialized || controller->ops == NULL ||
	    controller->ops->write_app == NULL)
		return PCIE_ERR_INVALID;
	return controller->ops->write_app(controller, offset, value);
}

/**
 * @brief Enable or disable writes to read-only DBI fields.
 *
 * @param[in,out] controller PCIe controller descriptor.
 * @param[in] enable true to allow read-only field writes.
 * @return PCIE_OK on success, PCIE_ERR_UNSUPPORTED when unavailable.
 */
int pcie_controller_dbi_ro_write_enable(struct pcie_controller *controller,
		bool enable)
{
	if (controller == NULL || !controller->initialized || controller->ops == NULL)
		return PCIE_ERR_INVALID;
	if (controller->ops->dbi_ro_write_enable == NULL)
		return PCIE_ERR_UNSUPPORTED;
	return controller->ops->dbi_ro_write_enable(controller, enable);
}

/**
 * @brief Enable or disable an endpoint BAR through the controller ops.
 *
 * @param[in,out] controller PCIe controller descriptor.
 * @param[in] function Endpoint function number.
 * @param[in] bar BAR number.
 * @param[in] enable true to enable the BAR, false to disable it.
 * @param[in] bar_64bit true when the BAR is 64-bit wide.
 * @return PCIE_OK on success, otherwise an error code.
 */
int pcie_controller_set_ep_bar(struct pcie_controller *controller,
		uint8_t function, uint8_t bar, bool enable, bool bar_64bit)
{
	if (controller == NULL || !controller->initialized || controller->ops == NULL)
		return PCIE_ERR_INVALID;
	if (controller->ops->set_ep_bar == NULL)
		return PCIE_ERR_UNSUPPORTED;
	return controller->ops->set_ep_bar(controller, function, bar, enable,
		bar_64bit);
}

/**
 * @brief Set the controller operating mode through the ops.
 *
 * @param[in,out] controller PCIe controller descriptor.
 * @param[in] mode PCIE_MODE_RC or PCIE_MODE_EP.
 * @return PCIE_OK on success, otherwise an error code.
 */
int pcie_controller_set_mode(struct pcie_controller *controller,
		enum pcie_mode mode)
{
	int ret;

	if (controller == NULL || !controller->initialized || controller->ops == NULL ||
	    controller->ops->set_mode == NULL ||
	    (mode != PCIE_MODE_RC && mode != PCIE_MODE_EP))
		return PCIE_ERR_INVALID;
	ret = controller->ops->set_mode(controller, mode);
	if (ret)
		return ret;
	controller->config.mode = mode;
	return PCIE_OK;
}

/**
 * @brief Program the link configuration through the ops.
 *
 * @param[in,out] controller PCIe controller descriptor.
 * @return PCIE_OK on success, otherwise an error code.
 */
int pcie_controller_set_link(struct pcie_controller *controller)
{
	if (controller == NULL || !controller->initialized || controller->ops == NULL ||
	    controller->ops->set_link == NULL)
		return PCIE_ERR_INVALID;
	return controller->ops->set_link(controller);
}

/**
 * @brief Retrain the link to a higher generation through the ops.
 *
 * @param[in,out] controller PCIe controller descriptor.
 * @param[in] link_gen Target link generation.
 * @return PCIE_OK on success, otherwise an error code.
 */
int pcie_controller_change_speed(struct pcie_controller *controller,
		uint8_t link_gen)
{
	if (controller == NULL || !controller->initialized || controller->ops == NULL ||
	    controller->ops->change_speed == NULL)
		return PCIE_ERR_INVALID;
	return controller->ops->change_speed(controller, link_gen);
}

/**
 * @brief Enable or disable the link training state machine through the ops.
 *
 * @param[in,out] controller PCIe controller descriptor.
 * @param[in] enable true to start training, false to stop it.
 * @return PCIE_OK on success, otherwise an error code.
 */
int pcie_controller_ltssm(struct pcie_controller *controller, bool enable)
{
	if (controller == NULL || !controller->initialized || controller->ops == NULL ||
	    controller->ops->ltssm == NULL)
		return PCIE_ERR_INVALID;
	return controller->ops->ltssm(controller, enable);
}

/**
 * @brief Report whether the link is up through the ops.
 *
 * @param[in] controller PCIe controller descriptor.
 * @return true when the link is up.
 */
bool pcie_controller_link_up(struct pcie_controller *controller)
{
	if (controller == NULL || !controller->initialized || controller->ops == NULL ||
	    controller->ops->link_up == NULL)
		return false;
	return controller->ops->link_up(controller);
}

/**
 * @brief Wait for the link to come up through the ops.
 *
 * @param[in] controller PCIe controller descriptor.
 * @param[in] timeout_us Timeout in microseconds.
 * @return PCIE_OK when the link is up, otherwise an error code.
 */
int pcie_controller_wait_link(struct pcie_controller *controller,
		uint32_t timeout_us)
{
	if (controller == NULL || !controller->initialized || controller->ops == NULL ||
	    controller->ops->wait_link == NULL)
		return PCIE_ERR_INVALID;
	return controller->ops->wait_link(controller, timeout_us);
}

/**
 * @brief Program an ATU window through the controller ops.
 *
 * @param[in,out] controller PCIe controller descriptor.
 * @param[in] region ATU window descriptor.
 * @return PCIE_OK on success, otherwise an error code.
 */
int pcie_controller_program_atu(struct pcie_controller *controller,
		const struct pcie_atu_region *region)
{
	if (controller == NULL || !controller->initialized || controller->ops == NULL ||
	    controller->ops->program_atu == NULL)
		return PCIE_ERR_INVALID;
	return controller->ops->program_atu(controller, region);
}

/**
 * @brief Disable an ATU window through the controller ops.
 *
 * @param[in] controller PCIe controller descriptor.
 * @param[in] direction PCIE_ATU_OUTBOUND or PCIE_ATU_INBOUND.
 * @param[in] index Window index.
 * @return PCIE_OK on success, otherwise an error code.
 */
int pcie_controller_disable_atu(struct pcie_controller *controller,
		enum pcie_atu_direction direction, uint8_t index)
{
	if (controller == NULL || !controller->initialized || controller->ops == NULL ||
	    controller->ops->disable_atu == NULL)
		return PCIE_ERR_INVALID;
	return controller->ops->disable_atu(controller, direction, index);
}

/**
 * @brief Find a PCIe capability in a function's configuration space.
 *
 * @param[in] controller PCIe controller descriptor.
 * @param[in] function_offset DBI offset of the function.
 * @param[in] capability Capability ID to search for.
 * @return The capability offset on success, otherwise an error code.
 */
int pcie_controller_find_capability(struct pcie_controller *controller,
		uint32_t function_offset, uint8_t capability)
{
	uint32_t pointer;
	uint32_t header;
	uint32_t next;
	unsigned int count;
	int ret;

	if (controller == NULL || !controller->initialized || capability == 0U ||
	    function_offset >= 0x800000U)
		return PCIE_ERR_INVALID;
	ret = pcie_controller_dbi_read(controller,
		function_offset + PCIE_CFG_CAP_PTR, 2U, &pointer);
	if (ret)
		return ret;
	pointer &= 0xffU;
	for (count = 0U; pointer != 0U && count < 48U; ++count) {
		if (pointer < 0x40U || pointer >= 0x1000U || (pointer & 0x3U) != 0U)
			return PCIE_ERR_UNSUPPORTED;
		ret = pcie_controller_dbi_read(controller,
			function_offset + pointer, 2U, &header);
		if (ret)
			return ret;
		if ((header & 0xffU) == capability)
			return (int)pointer;
		next = (header >> 8) & 0xffU;
		pointer = next;
	}
	return PCIE_ERR_UNSUPPORTED;
}

/**
 * @brief Find an extended capability in a function's configuration space.
 *
 * @param[in] controller PCIe controller descriptor.
 * @param[in] function_offset DBI offset of the function.
 * @param[in] capability Extended capability ID to search for.
 * @return The capability offset on success, otherwise an error code.
 */
int pcie_controller_find_ext_capability(struct pcie_controller *controller,
		uint32_t function_offset, uint16_t capability)
{
	uint32_t header;
	uint32_t next;
	uint32_t offset;
	unsigned int count;
	int ret;

	if (controller == NULL || !controller->initialized || capability == 0U ||
	    function_offset >= 0x800000U)
		return PCIE_ERR_INVALID;
	offset = PCIE_EXT_CAP_START;
	for (count = 0U; count < 1024U; ++count) {
		if (offset < PCIE_EXT_CAP_START || offset >= 0x1000U ||
		    (offset & 0x3U) != 0U)
			return PCIE_ERR_UNSUPPORTED;
		ret = pcie_controller_dbi_read(controller,
			function_offset + offset, 4U, &header);
		if (ret)
			return ret;
		if ((header & PCIE_EXT_CAP_ID_MASK) == capability)
			return (int)offset;
		next = (header & PCIE_EXT_CAP_NEXT_MASK) >> 20;
		if (next == 0U || next <= offset)
			return PCIE_ERR_UNSUPPORTED;
		offset = next;
	}
	return PCIE_ERR_UNSUPPORTED;
}

/**
 * @brief Perform a configuration-space read or write access.
 *
 * Routes accesses to the root port directly through the DBI space and
 * translates accesses to downstream devices through the configuration ATU
 * window.
 *
 * @param[in,out] controller PCIe controller descriptor.
 * @param[in] bdf Bus/device/function of the target.
 * @param[in] offset Register offset within the configuration space.
 * @param[in] size Access width in bytes.
 * @param[out] value For a read, receives the read value.
 * @param[in] write true for a write, false for a read.
 * @param[in] write_value Value to write for a write access.
 * @return PCIE_OK on success, otherwise an error code.
 */
static int pcie_controller_cfg_access(struct pcie_controller *controller,
		uint32_t bdf, uint32_t offset, uint8_t size, uint32_t *value,
		bool write, uint32_t write_value)
{
	struct pcie_atu_region region;
	uintptr_t address;
	uint32_t bus;

	if (controller == NULL || !controller->initialized || value == NULL ||
	    !pcie_access_size_valid(size) ||
	    (offset & ((uint32_t)size - 1U)) != 0U ||
	    (uint64_t)offset + size > 0x1000U)
		return PCIE_ERR_INVALID;
	if ((bdf & ~0x00ffff00U) != 0U ||
	    PCIE_BDF_DEVICE(bdf) >= 32U || PCIE_BDF_FUNCTION(bdf) >= 8U)
		return PCIE_ERR_INVALID;
	bus = PCIE_BDF_BUS(bdf);
	if (bus == 0U && PCIE_BDF_DEVICE(bdf) == 0U &&
	    PCIE_BDF_FUNCTION(bdf) == 0U) {
		if (write)
			return pcie_controller_dbi_write(controller, offset, size, write_value);
		return pcie_controller_dbi_read(controller, offset, size, value);
	}
	if (bus == 0U)
		return PCIE_ERR_NO_DEVICE;
	if (controller->config.cfg_cpu_addr == 0U || controller->config.cfg_size < 0x1000U)
		return PCIE_ERR_INVALID;
	if (!pcie_controller_link_up(controller))
		return PCIE_ERR_NO_DEVICE;
	region = (struct pcie_atu_region){
		.direction = PCIE_ATU_OUTBOUND,
		.type = bus == 1U ? PCIE_ATU_TYPE_CFG0 : PCIE_ATU_TYPE_CFG1,
		.index = 0U,
		.cpu_addr = controller->config.cfg_cpu_addr,
		.pci_addr = PCIE_BDF_CONFIG_TARGET(bdf),
		.size = controller->config.cfg_size,
	};
	{
		int ret = pcie_controller_program_atu(controller, &region);

		if (ret)
			return ret;
	}
	address = controller->config.cfg_cpu_addr + offset;
	if (write) {
		if (size == 1U)
			writeb(write_value, address);
		else if (size == 2U)
			writew(write_value, address);
		else
			writel(write_value, address);
		return PCIE_OK;
	}
	if (size == 1U)
		*value = readb(address);
	else if (size == 2U)
		*value = readw(address);
	else
		*value = readl(address);
	return PCIE_OK;
}

/**
 * @brief Read a configuration-space field.
 *
 * @param[in,out] controller PCIe controller descriptor.
 * @param[in] bdf Bus/device/function of the target.
 * @param[in] offset Register offset within the configuration space.
 * @param[in] size Access width in bytes.
 * @param[out] value Receives the read value.
 * @return PCIE_OK on success, otherwise an error code.
 */
int pcie_controller_cfg_read(struct pcie_controller *controller,
		uint32_t bdf, uint32_t offset, uint8_t size, uint32_t *value)
{
	return pcie_controller_cfg_access(controller, bdf, offset, size, value,
		false, 0U);
}

/**
 * @brief Write a configuration-space field.
 *
 * @param[in,out] controller PCIe controller descriptor.
 * @param[in] bdf Bus/device/function of the target.
 * @param[in] offset Register offset within the configuration space.
 * @param[in] size Access width in bytes.
 * @param[in] value Value to write.
 * @return PCIE_OK on success, otherwise an error code.
 */
int pcie_controller_cfg_write(struct pcie_controller *controller,
		uint32_t bdf, uint32_t offset, uint8_t size, uint32_t value)
{
	uint32_t ignored;

	ignored = 0U;
	return pcie_controller_cfg_access(controller, bdf, offset, size, &ignored,
		true, value);
}
