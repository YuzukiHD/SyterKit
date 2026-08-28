/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stdint.h>

#include <drivers/pcie/ep/pcie-ep.h>
#include <dt-compatible/pcie-dt.h>

#define PCIE_EP_MSI_MMC_MASK            (0x7U << 1)
#define PCIE_EP_BAR_MIN_SIZE             (1ULL << 12)
#define PCIE_EP_REBAR_MIN_SIZE           (1ULL << 20)
#define PCIE_EP_BAR_MAX_SIZE             (1ULL << 39)

static int pcie_ep_bar_size_valid(uint64_t size)
{
	if (size == 0U)
		return PCIE_ERR_INVALID;
	if (size < PCIE_EP_BAR_MIN_SIZE || size > PCIE_EP_BAR_MAX_SIZE ||
	    (size & (size - 1U)) != 0U)
		return PCIE_ERR_UNSUPPORTED;
	return PCIE_OK;
}

static int pcie_ep_rebar_entry(uint8_t bar, uint32_t flags,
		uint32_t *entry)
{
	if (entry == NULL)
		return PCIE_ERR_INVALID;
	if ((flags & PCIE_EP_BAR_IO) != 0U)
		return PCIE_ERR_UNSUPPORTED;
	if ((flags & PCIE_EP_BAR_64BIT) != 0U) {
		if (bar == 0U || bar == 4U)
			*entry = bar == 0U ? 0U : 3U;
		else
			return PCIE_ERR_UNSUPPORTED;
	} else if (bar == 2U || bar == 3U) {
		*entry = (uint32_t)bar - 1U;
	} else {
		return PCIE_ERR_UNSUPPORTED;
	}
	return PCIE_OK;
}

static int pcie_ep_rebar_size(uint64_t size, uint32_t *value)
{
	uint64_t rebar_size;
	uint32_t shift;
	int ret;

	if (value == NULL)
		return PCIE_ERR_INVALID;
	ret = pcie_ep_bar_size_valid(size);
	if (ret)
		return ret;
	if (size < PCIE_EP_REBAR_MIN_SIZE)
		return PCIE_ERR_UNSUPPORTED;
	rebar_size = PCIE_EP_REBAR_MIN_SIZE;
	shift = 0U;
	while (rebar_size < size) {
		rebar_size <<= 1;
		++shift;
	}
	*value = shift << PCIE_REBAR_CTRL_BAR_SIZE_SHIFT;
	return PCIE_OK;
}

static int pcie_ep_function_offset(const struct pcie *pcie, uint8_t function,
		uint32_t *offset)
{
	if (pcie == NULL || offset == NULL || function >= 8U ||
	    pcie->controller.config.ep_function_stride == 0U)
		return PCIE_ERR_INVALID;
	if ((uint64_t)function * pcie->controller.config.ep_function_stride >
		0xffffffffULL)
		return PCIE_ERR_INVALID;
	*offset = (uint32_t)function * pcie->controller.config.ep_function_stride;
	return PCIE_OK;
}

int pcie_ep_init(struct pcie *pcie, const struct pcie_config *config)
{
	uint32_t value;
	int ret;

	ret = pcie_init(pcie, config);
	if (ret)
		return ret;
	ret = pcie_controller_dbi_read(&pcie->controller, PCIE_CFG_COMMAND, 4U, &value);
	if (ret)
		goto fail;
	value &= 0xffff0000U;
	value |= PCIE_CFG_COMMAND_IO | PCIE_CFG_COMMAND_MEMORY |
		PCIE_CFG_COMMAND_MASTER | PCIE_CFG_COMMAND_SERR;
	ret = pcie_controller_dbi_write(&pcie->controller, PCIE_CFG_COMMAND, 4U, value);
	if (ret)
		goto fail;
	return PCIE_OK;

fail:
	pcie_exit(pcie);
	return ret;
}

int pcie_ep_init_dt(struct pcie *pcie, int node)
{
	struct pcie_config config;

	if (sunxi_pcie_dt_read_config(&config, node) != DRIVER_OK ||
	    config.mode != PCIE_MODE_EP)
		return PCIE_ERR_INVALID;
	return pcie_ep_init(pcie, &config);
}

int pcie_ep_write_header(struct pcie *pcie, uint8_t function,
		const struct pcie_ep_header *header)
{
	uint32_t function_offset;
	uint32_t class_code;
	int ret;

	if (pcie == NULL || header == NULL || !pcie->initialized ||
	    pcie->mode != PCIE_MODE_EP)
		return PCIE_ERR_INVALID;
	ret = pcie_ep_function_offset(pcie, function, &function_offset);
	if (ret)
		return ret;
	class_code = (uint32_t)header->subclass | ((uint32_t)header->base_class << 8);
	ret = pcie_controller_dbi_ro_write_enable(&pcie->controller, true);
	if (ret)
		return ret;
	ret = pcie_controller_dbi_write(&pcie->controller, function_offset + 0x00U,
		2U, header->vendor_id);
	if (!ret)
		ret = pcie_controller_dbi_write(&pcie->controller, function_offset + 0x02U,
		2U, header->device_id);
	if (!ret)
		ret = pcie_controller_dbi_write(&pcie->controller, function_offset + 0x08U,
		1U, header->revision_id);
	if (!ret)
		ret = pcie_controller_dbi_write(&pcie->controller, function_offset + 0x09U,
		1U, header->prog_if);
	if (!ret)
		ret = pcie_controller_dbi_write(&pcie->controller, function_offset + 0x0aU,
		2U, class_code);
	if (!ret)
		ret = pcie_controller_dbi_write(&pcie->controller, function_offset + 0x0cU,
		1U, header->cache_line_size);
	if (!ret)
		ret = pcie_controller_dbi_write(&pcie->controller, function_offset + 0x2cU,
		2U, header->subsystem_vendor_id);
	if (!ret)
		ret = pcie_controller_dbi_write(&pcie->controller, function_offset + 0x2eU,
		2U, header->subsystem_id);
	if (!ret)
		ret = pcie_controller_dbi_write(&pcie->controller, function_offset + 0x3dU,
		1U, header->interrupt_pin);
	if (pcie_controller_dbi_ro_write_enable(&pcie->controller, false) != PCIE_OK)
		return PCIE_ERR_IO;
	return ret;
}

int pcie_ep_set_bar(struct pcie *pcie, uint8_t function,
		const struct pcie_ep_bar *bar)
{
	struct pcie_atu_region region;
	uint32_t function_offset;
	uint32_t bar_value;
	uint32_t rebar_offset;
	uint32_t rebar_entry;
	uint32_t rebar_ctrl;
	int cleanup_ret;
	int ret;

	if (pcie == NULL || bar == NULL || !pcie->initialized ||
	    pcie->mode != PCIE_MODE_EP || bar->bar >= 6U || bar->size == 0U ||
	    (bar->flags & ~PCIE_EP_BAR_FLAGS_MASK) != 0U)
		return PCIE_ERR_INVALID;
	if ((bar->flags & PCIE_EP_BAR_IO) != 0U)
		return PCIE_ERR_UNSUPPORTED;
	if ((bar->flags & PCIE_EP_BAR_64BIT) != 0U &&
	    (bar->bar != 0U && bar->bar != 4U))
		return PCIE_ERR_UNSUPPORTED;
	ret = pcie_ep_bar_size_valid(bar->size);
	if (ret)
		return ret;
	if ((bar->phys_addr & (bar->size - 1U)) != 0U)
		return PCIE_ERR_INVALID;
	ret = pcie_ep_function_offset(pcie, function, &function_offset);
	if (ret)
		return ret;
	region = (struct pcie_atu_region){
		.direction = PCIE_ATU_INBOUND,
		.type = PCIE_ATU_TYPE_MEM,
		.index = bar->bar,
		.function = function,
		.bar = bar->bar,
		.function_match = true,
		.bar_match = true,
		.cpu_addr = bar->phys_addr,
		.size = bar->size,
	};
	ret = pcie_controller_program_atu(&pcie->controller, &region);
	if (ret)
		return ret;
	bar_value = bar->flags & PCIE_EP_BAR_FLAGS_MASK;
	ret = pcie_controller_dbi_ro_write_enable(&pcie->controller, true);
	if (ret)
		goto disable_atu;
	ret = pcie_controller_dbi_write(&pcie->controller,
		function_offset + PCIE_CFG_BAR0 + 4U * bar->bar, 4U, bar_value);
	if (!ret && (bar->flags & PCIE_EP_BAR_64BIT) != 0U)
		ret = pcie_controller_dbi_write(&pcie->controller,
			function_offset + PCIE_CFG_BAR0 + 4U * bar->bar + 4U, 4U, 0U);
	if (!ret) {
		ret = pcie_controller_find_ext_capability(&pcie->controller,
			function_offset, PCIE_EXT_CAP_ID_REBAR);
		if (ret >= 0) {
			rebar_offset = (uint32_t)ret;
			ret = pcie_ep_rebar_entry(bar->bar, bar->flags, &rebar_entry);
			if (ret == PCIE_ERR_UNSUPPORTED) {
				ret = PCIE_OK;
			} else if (!ret) {
				ret = pcie_ep_rebar_size(bar->size, &bar_value);
				if (!ret)
					ret = pcie_controller_dbi_write(&pcie->controller,
						function_offset + rebar_offset + PCIE_REBAR_CAP +
						rebar_entry * PCIE_REBAR_ENTRY_STRIDE, 4U,
						PCIE_REBAR_CAP_SIZES);
				if (!ret) {
					rebar_ctrl = 0U;
					ret = pcie_controller_dbi_read(&pcie->controller,
						function_offset + rebar_offset + PCIE_REBAR_CTRL +
						rebar_entry * PCIE_REBAR_ENTRY_STRIDE, 4U,
						&rebar_ctrl);
				}
				if (!ret) {
					rebar_ctrl = (rebar_ctrl &
						~PCIE_REBAR_CTRL_BAR_SIZE_MASK) |
						(bar_value & PCIE_REBAR_CTRL_BAR_SIZE_MASK);
					ret = pcie_controller_dbi_write(&pcie->controller,
						function_offset + rebar_offset + PCIE_REBAR_CTRL +
						rebar_entry * PCIE_REBAR_ENTRY_STRIDE, 4U,
						rebar_ctrl);
				}
			}
		} else if (ret == PCIE_ERR_UNSUPPORTED) {
			ret = PCIE_OK;
		}
	}
	if (!ret)
		ret = pcie_controller_set_ep_bar(&pcie->controller, function,
			bar->bar, true, (bar->flags & PCIE_EP_BAR_64BIT) != 0U);
	if (pcie_controller_dbi_ro_write_enable(&pcie->controller, false) != PCIE_OK)
		ret = PCIE_ERR_IO;
	if (ret)
		goto disable_atu;
	return ret;

disable_atu:
	cleanup_ret = pcie_controller_disable_atu(&pcie->controller,
		PCIE_ATU_INBOUND, bar->bar);
	if (cleanup_ret != PCIE_OK && ret == PCIE_OK)
		ret = cleanup_ret;
	return ret;
}

int pcie_ep_clear_bar(struct pcie *pcie, uint8_t function, uint8_t bar)
{
	uint32_t function_offset;
	uint32_t bar_value;
	bool bar_64bit;
	int cleanup_ret;
	int ret;

	if (pcie == NULL || !pcie->initialized || pcie->mode != PCIE_MODE_EP || bar >= 6U)
		return PCIE_ERR_INVALID;
	ret = pcie_ep_function_offset(pcie, function, &function_offset);
	if (ret)
		return ret;
	ret = pcie_controller_dbi_read(&pcie->controller,
		function_offset + PCIE_CFG_BAR0 + 4U * bar, 4U, &bar_value);
	if (ret)
		return ret;
	bar_64bit = (bar_value & PCIE_EP_BAR_64BIT) != 0U && bar < 5U;
	ret = pcie_controller_dbi_ro_write_enable(&pcie->controller, true);
	if (ret)
		return ret;
	ret = pcie_controller_dbi_write(&pcie->controller,
		function_offset + PCIE_CFG_BAR0 + 4U * bar, 4U, 0U);
	if (!ret && bar_64bit)
		ret = pcie_controller_dbi_write(&pcie->controller,
			function_offset + PCIE_CFG_BAR0 + 4U * (bar + 1U), 4U, 0U);
	if (!ret)
		ret = pcie_controller_set_ep_bar(&pcie->controller, function, bar,
			false, bar_64bit);
	if (pcie_controller_dbi_ro_write_enable(&pcie->controller, false) != PCIE_OK)
		ret = PCIE_ERR_IO;
	cleanup_ret = pcie_controller_disable_atu(&pcie->controller,
		PCIE_ATU_INBOUND, bar);
	if (cleanup_ret != PCIE_OK && ret == PCIE_OK)
		ret = cleanup_ret;
	return ret;
}

int pcie_ep_program_inbound(struct pcie *pcie, uint8_t function,
		uint8_t index, enum pcie_atu_type type, uint64_t local_addr,
		uint64_t pci_addr, uint64_t size)
{
	struct pcie_atu_region region;

	if (pcie == NULL || !pcie->initialized || pcie->mode != PCIE_MODE_EP ||
		function >= 8U)
		return PCIE_ERR_INVALID;
	region = (struct pcie_atu_region){
		.direction = PCIE_ATU_INBOUND,
		.type = type,
		.index = index,
		.function = function,
		.function_match = true,
		.cpu_addr = local_addr,
		.pci_addr = pci_addr,
		.size = size,
	};
	return pcie_controller_program_atu(&pcie->controller, &region);
}

int pcie_ep_program_outbound(struct pcie *pcie, uint8_t index,
		enum pcie_atu_type type, uint64_t local_addr, uint64_t pci_addr,
		uint64_t size)
{
	struct pcie_atu_region region;

	if (pcie == NULL || !pcie->initialized || pcie->mode != PCIE_MODE_EP)
		return PCIE_ERR_INVALID;
	region = (struct pcie_atu_region){
		.direction = PCIE_ATU_OUTBOUND,
		.type = type,
		.index = index,
		.cpu_addr = local_addr,
		.pci_addr = pci_addr,
		.size = size,
	};
	return pcie_controller_program_atu(&pcie->controller, &region);
}

int pcie_ep_configure_msi(struct pcie *pcie, uint8_t function,
		uint8_t multiple_message_capable)
{
	uint32_t function_offset;
	uint32_t flags;
	int capability;
	int ret;

	if (pcie == NULL || !pcie->initialized || pcie->mode != PCIE_MODE_EP ||
	    multiple_message_capable > 5U)
		return PCIE_ERR_INVALID;
	ret = pcie_ep_function_offset(pcie, function, &function_offset);
	if (ret)
		return ret;
	capability = pcie_controller_find_capability(&pcie->controller, function_offset,
		PCIE_CAP_ID_MSI);
	if (capability < 0)
		return capability;
	ret = pcie_controller_dbi_read(&pcie->controller,
		function_offset + (uint32_t)capability + PCIE_CAP_MSI_FLAGS, 2U, &flags);
	if (ret)
		return ret;
	flags &= ~PCIE_EP_MSI_MMC_MASK;
	flags |= ((uint32_t)multiple_message_capable << 1) & PCIE_EP_MSI_MMC_MASK;
	ret = pcie_controller_dbi_ro_write_enable(&pcie->controller, true);
	if (ret)
		return ret;
	ret = pcie_controller_dbi_write(&pcie->controller,
		function_offset + (uint32_t)capability + PCIE_CAP_MSI_FLAGS, 2U, flags);
	if (pcie_controller_dbi_ro_write_enable(&pcie->controller, false) != PCIE_OK)
		return PCIE_ERR_IO;
	return ret;
}

int pcie_ep_start(struct pcie *pcie)
{
	if (pcie == NULL || !pcie->initialized || pcie->mode != PCIE_MODE_EP)
		return PCIE_ERR_INVALID;
	return pcie_controller_ltssm(&pcie->controller, true);
}

int pcie_ep_stop(struct pcie *pcie)
{
	if (pcie == NULL || !pcie->initialized || pcie->mode != PCIE_MODE_EP)
		return PCIE_ERR_INVALID;
	return pcie_controller_ltssm(&pcie->controller, false);
}

bool pcie_ep_link_up(struct pcie *pcie)
{
	if (pcie == NULL || !pcie->initialized || pcie->mode != PCIE_MODE_EP)
		return false;
	return pcie_controller_link_up(&pcie->controller);
}
