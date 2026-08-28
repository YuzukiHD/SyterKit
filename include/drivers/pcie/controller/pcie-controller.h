/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_PCIE_CONTROLLER_H__
#define __DRIVERS_PCIE_CONTROLLER_H__

#include <stdbool.h>
#include <stdint.h>

#define PCIE_OK                         0
#define PCIE_ERR_INVALID                (-1)
#define PCIE_ERR_TIMEOUT                (-2)
#define PCIE_ERR_UNSUPPORTED            (-3)
#define PCIE_ERR_NO_DEVICE              (-4)
#define PCIE_ERR_IO                     (-5)

enum pcie_mode {
	PCIE_MODE_RC = 0,
	PCIE_MODE_EP = 1,
};

enum pcie_atu_direction {
	PCIE_ATU_OUTBOUND = 0,
	PCIE_ATU_INBOUND = 1,
};

enum pcie_atu_type {
	PCIE_ATU_TYPE_MEM = 0x0,
	PCIE_ATU_TYPE_IO = 0x2,
	PCIE_ATU_TYPE_CFG0 = 0x4,
	PCIE_ATU_TYPE_CFG1 = 0x5,
};

/* PCI configuration space offsets used by the core and the RC/EP layers. */
#define PCIE_CFG_VENDOR_ID              0x00U
#define PCIE_CFG_COMMAND                0x04U
#define PCIE_CFG_CLASS_REV              0x08U
#define PCIE_CFG_CLASS_CODE             0x0aU
#define PCIE_CFG_HEADER_TYPE            0x0eU
#define PCIE_CFG_BAR0                   0x10U
#define PCIE_CFG_CAP_PTR                0x34U
#define PCIE_CFG_INTERRUPT_LINE         0x3cU
#define PCIE_CFG_PRIMARY_BUS            0x18U

#define PCIE_CFG_COMMAND_IO             (1U << 0)
#define PCIE_CFG_COMMAND_MEMORY         (1U << 1)
#define PCIE_CFG_COMMAND_MASTER         (1U << 2)
#define PCIE_CFG_COMMAND_SERR           (1U << 8)

#define PCIE_CAP_ID_MSI                 0x05U
#define PCIE_CAP_ID_EXPRESS             0x10U
#define PCIE_CAP_MSI_FLAGS              0x02U
#define PCIE_CAP_EXP_LINK_CAP           0x0cU
#define PCIE_CAP_EXP_LINK_CTRL2         0x30U
#define PCIE_CAP_EXP_LINK_SPEED_MASK    0x0fU

#define PCIE_EXT_CAP_START              0x100U
#define PCIE_EXT_CAP_ID_MASK            0x0000ffffU
#define PCIE_EXT_CAP_NEXT_MASK          0xfff00000U
#define PCIE_EXT_CAP_ID_REBAR           0x15U
#define PCIE_REBAR_CAP                  0x04U
#define PCIE_REBAR_CAP_SIZES            0x00fffff0U
#define PCIE_REBAR_CTRL                 0x08U
#define PCIE_REBAR_CTRL_BAR_SIZE_MASK   0x00001f00U
#define PCIE_REBAR_CTRL_BAR_SIZE_SHIFT  8U
#define PCIE_REBAR_ENTRY_STRIDE         0x08U

#define PCIE_HEADER_TYPE_NORMAL         0x00U
#define PCIE_HEADER_TYPE_BRIDGE         0x01U

#define PCIE_DW_PORT_LINK_CONTROL       0x0710U
#define PCIE_DW_LINK_WIDTH_SPEED_CTRL   0x080cU
#define PCIE_DW_MISC_CONTROL_1          0x08bcU
#define PCIE_DW_MSI_ADDR_LO             0x0820U
#define PCIE_DW_MSI_ADDR_HI             0x0824U
#define PCIE_DW_LTSSM_CTRL              0x0c00U
#define PCIE_DW_LINK_STATUS             0x0e0cU

#define PCIE_DW_LINK_UP_MASK            0x3U
#define PCIE_DW_LTSSM_ENABLE            (1U << 0)
#define PCIE_DW_SPEED_CHANGE            (1U << 17)
#define PCIE_DW_LINK_WIDTH_MASK         (0x1ffU << 8)
#define PCIE_DW_LINK_WIDTH_1            (0x001U << 8)
#define PCIE_DW_LINK_WIDTH_2            (0x002U << 8)
#define PCIE_DW_LINK_WIDTH_4            (0x004U << 8)
#define PCIE_DW_PORT_LINK_MODE_MASK     (0x3fU << 16)
#define PCIE_DW_PORT_LINK_MODE_1        (0x001U << 16)
#define PCIE_DW_PORT_LINK_MODE_2        (0x003U << 16)
#define PCIE_DW_PORT_LINK_MODE_4        (0x007U << 16)
#define PCIE_DW_DBI_RO_WRITE_ENABLE     (1U << 0)

#define PCIE_DW_ATU_BASE                0x300000U
#define PCIE_DW_ATU_REGION_STRIDE       0x200U
#define PCIE_DW_ATU_OUTBOUND_OFFSET     0x000U
#define PCIE_DW_ATU_INBOUND_OFFSET      0x100U
#define PCIE_DW_ATU_CTRL1               0x000U
#define PCIE_DW_ATU_CTRL2               0x004U
#define PCIE_DW_ATU_BASE_LO             0x008U
#define PCIE_DW_ATU_BASE_HI             0x00cU
#define PCIE_DW_ATU_LIMIT_LO            0x010U
#define PCIE_DW_ATU_TARGET_LO           0x014U
#define PCIE_DW_ATU_TARGET_HI           0x018U
#define PCIE_DW_ATU_CTRL3               0x01cU
#define PCIE_DW_ATU_LIMIT_HI            0x020U
#define PCIE_DW_ATU_ALIGNMENT            0x1000U
#define PCIE_DW_ATU_ENABLE              (1U << 31)
#define PCIE_DW_ATU_BAR_MODE            (1U << 30)
#define PCIE_DW_ATU_FUNC_MATCH          (1U << 19)
#define PCIE_DW_ATU_INCREASE_REGION_SIZE (1U << 13)
#define PCIE_DW_ATU_FUNC_NUM(function)  (((uint32_t)(function) & 0x7U) << 20)

#define PCIE_BDF(bus, device, function) \
	((((uint32_t)(bus) & 0xffU) << 16) | \
	 (((uint32_t)(device) & 0x1fU) << 11) | \
	 (((uint32_t)(function) & 0x7U) << 8))
#define PCIE_BDF_BUS(bdf)               (((uint32_t)(bdf) >> 16) & 0xffU)
#define PCIE_BDF_DEVICE(bdf)            (((uint32_t)(bdf) >> 11) & 0x1fU)
#define PCIE_BDF_FUNCTION(bdf)          (((uint32_t)(bdf) >> 8) & 0x7U)
#define PCIE_BDF_CONFIG_TARGET(bdf)     \
	((PCIE_BDF_BUS(bdf) << 24) | \
	 (PCIE_BDF_DEVICE(bdf) << 19) | \
	 (PCIE_BDF_FUNCTION(bdf) << 16))

struct pcie_controller;

struct pcie_controller_config {
	uintptr_t dbi_base;
	uintptr_t app_base;
	uint32_t dbi_size;
	uint32_t app_size;

	uintptr_t cfg_cpu_addr;
	/* Config requests use a BDF-encoded iATU target, not this base. */
	uint64_t cfg_pci_addr;
	uint32_t cfg_size;
	uintptr_t mem_cpu_addr;
	uint64_t mem_pci_addr;
	uint32_t mem_size;
	uintptr_t io_cpu_addr;
	uint64_t io_pci_addr;
	uint32_t io_size;

	enum pcie_mode mode;
	uint8_t lanes;
	uint8_t link_gen;
	uint8_t max_lanes;
	uint8_t max_link_gen;
	uint8_t num_ob_windows;
	uint8_t num_ib_windows;
	/* Endpoint function configuration-space stride in the controller DBI. */
	uint32_t ep_function_stride;
	uint32_t timeout_us;
};

struct pcie_atu_region {
	enum pcie_atu_direction direction;
	enum pcie_atu_type type;
	uint8_t index;
	uint8_t function;
	uint8_t bar;
	bool function_match;
	bool bar_match;
	uint64_t cpu_addr;
	uint64_t pci_addr;
	uint64_t size;
};

struct pcie_controller_ops {
	int (*init)(struct pcie_controller *controller);
	void (*exit)(struct pcie_controller *controller);
	int (*read_dbi)(struct pcie_controller *controller, uint32_t offset,
			uint8_t size, uint32_t *value);
	int (*write_dbi)(struct pcie_controller *controller, uint32_t offset,
			uint8_t size, uint32_t value);
	int (*dbi_ro_write_enable)(struct pcie_controller *controller,
			bool enable);
	int (*set_ep_bar)(struct pcie_controller *controller, uint8_t function,
			uint8_t bar, bool enable, bool bar_64bit);
	int (*read_app)(struct pcie_controller *controller, uint32_t offset,
			uint32_t *value);
	int (*write_app)(struct pcie_controller *controller, uint32_t offset,
			uint32_t value);
	int (*set_mode)(struct pcie_controller *controller, enum pcie_mode mode);
	int (*set_link)(struct pcie_controller *controller);
	int (*change_speed)(struct pcie_controller *controller, uint8_t link_gen);
	int (*ltssm)(struct pcie_controller *controller, bool enable);
	bool (*link_up)(struct pcie_controller *controller);
	int (*wait_link)(struct pcie_controller *controller, uint32_t timeout_us);
	int (*program_atu)(struct pcie_controller *controller,
			const struct pcie_atu_region *region);
	int (*disable_atu)(struct pcie_controller *controller,
			enum pcie_atu_direction direction, uint8_t index);
};

struct pcie_controller {
	struct pcie_controller_config config;
	const struct pcie_controller_ops *ops;
	bool initialized;
};

int pcie_controller_init(struct pcie_controller *controller,
		const struct pcie_controller_config *config);
int pcie_controller_init_with_ops(struct pcie_controller *controller,
		const struct pcie_controller_config *config,
		const struct pcie_controller_ops *ops);
void pcie_controller_exit(struct pcie_controller *controller);

int pcie_controller_dbi_read(struct pcie_controller *controller,
		uint32_t offset, uint8_t size, uint32_t *value);
int pcie_controller_dbi_write(struct pcie_controller *controller,
		uint32_t offset, uint8_t size, uint32_t value);
int pcie_controller_app_read(struct pcie_controller *controller,
		uint32_t offset, uint32_t *value);
int pcie_controller_app_write(struct pcie_controller *controller,
		uint32_t offset, uint32_t value);
int pcie_controller_dbi_ro_write_enable(struct pcie_controller *controller,
		bool enable);
int pcie_controller_set_ep_bar(struct pcie_controller *controller,
		uint8_t function, uint8_t bar, bool enable, bool bar_64bit);
int pcie_controller_set_mode(struct pcie_controller *controller,
		enum pcie_mode mode);
int pcie_controller_set_link(struct pcie_controller *controller);
int pcie_controller_change_speed(struct pcie_controller *controller,
		uint8_t link_gen);
int pcie_controller_ltssm(struct pcie_controller *controller, bool enable);
bool pcie_controller_link_up(struct pcie_controller *controller);
int pcie_controller_wait_link(struct pcie_controller *controller,
		uint32_t timeout_us);
int pcie_controller_program_atu(struct pcie_controller *controller,
		const struct pcie_atu_region *region);
int pcie_controller_disable_atu(struct pcie_controller *controller,
		enum pcie_atu_direction direction, uint8_t index);

int pcie_controller_cfg_read(struct pcie_controller *controller,
		uint32_t bdf, uint32_t offset, uint8_t size, uint32_t *value);
int pcie_controller_cfg_write(struct pcie_controller *controller,
		uint32_t bdf, uint32_t offset, uint8_t size, uint32_t value);
int pcie_controller_find_capability(struct pcie_controller *controller,
		uint32_t function_offset, uint8_t capability);
int pcie_controller_find_ext_capability(struct pcie_controller *controller,
		uint32_t function_offset, uint16_t capability);

#endif /* __DRIVERS_PCIE_CONTROLLER_H__ */
