/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file pcie-controller.h
 * @brief PCIe controller core, register access and ATU programming.
 *
 * Defines the PCIe controller configuration, ATU region description, the
 * controller operations table and the register-level helpers shared by the
 * RC and EP layers.
 */

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

/**
 * @enum pcie_mode
 * @brief PCIe controller operating mode.
 */
enum pcie_mode {
	PCIE_MODE_RC = 0, /**< Root Complex mode. */
	PCIE_MODE_EP = 1, /**< Endpoint mode. */
};

/**
 * @enum pcie_atu_direction
 * @brief Address translation unit (ATU) window direction.
 */
enum pcie_atu_direction {
	PCIE_ATU_OUTBOUND = 0, /**< Outbound (CPU to PCI) translation. */
	PCIE_ATU_INBOUND = 1, /**< Inbound (PCI to CPU) translation. */
};

/**
 * @enum pcie_atu_type
 * @brief Address translation unit (ATU) window type.
 */
enum pcie_atu_type {
	PCIE_ATU_TYPE_MEM = 0x0, /**< Memory window. */
	PCIE_ATU_TYPE_IO = 0x2, /**< IO window. */
	PCIE_ATU_TYPE_CFG0 = 0x4, /**< Type 0 configuration window. */
	PCIE_ATU_TYPE_CFG1 = 0x5, /**< Type 1 configuration window. */
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

/**
 * @struct pcie_controller_config
 * @brief Hardware configuration for the PCIe controller core.
 */
struct pcie_controller_config {
	uintptr_t dbi_base; /**< Base address of the DBI register block. */
	uintptr_t app_base; /**< Base address of the application register block. */
	uint32_t dbi_size; /**< Size of the DBI register block, in bytes. */
	uint32_t app_size; /**< Size of the application register block, in bytes. */

	uintptr_t cfg_cpu_addr; /**< CPU address of the configuration window. */
	/* Config requests use a BDF-encoded iATU target, not this base. */
	uint64_t cfg_pci_addr; /**< PCI address of the configuration window. */
	uint32_t cfg_size; /**< Size of the configuration window, in bytes. */
	uintptr_t mem_cpu_addr; /**< CPU address of the memory window. */
	uint64_t mem_pci_addr; /**< PCI address of the memory window. */
	uint32_t mem_size; /**< Size of the memory window, in bytes. */
	uintptr_t io_cpu_addr; /**< CPU address of the IO window. */
	uint64_t io_pci_addr; /**< PCI address of the IO window. */
	uint32_t io_size; /**< Size of the IO window, in bytes. */

	enum pcie_mode mode; /**< Controller operating mode, RC or EP. */
	uint8_t lanes; /**< Number of active lanes. */
	uint8_t link_gen; /**< Active link generation. */
	uint8_t max_lanes; /**< Maximum number of supported lanes. */
	uint8_t max_link_gen; /**< Maximum supported link generation. */
	uint8_t num_ob_windows; /**< Number of outbound ATU windows. */
	uint8_t num_ib_windows; /**< Number of inbound ATU windows. */
	/* Endpoint function configuration-space stride in the controller DBI. */
	uint32_t ep_function_stride; /**< Endpoint function configuration-space stride. */
	uint32_t timeout_us; /**< Timeout for controller operations, in microseconds. */
};

/**
 * @struct pcie_atu_region
 * @brief Description of one address translation unit (ATU) window.
 */
struct pcie_atu_region {
	enum pcie_atu_direction direction; /**< Window direction (outbound/inbound). */
	enum pcie_atu_type type; /**< Window type (memory, IO or config). */
	uint8_t index; /**< ATU window index. */
	uint8_t function; /**< Endpoint function for function-matched windows. */
	uint8_t bar; /**< BAR for bar-matched windows. */
	bool function_match; /**< Whether the window matches by function. */
	bool bar_match; /**< Whether the window matches by BAR. */
	uint64_t cpu_addr; /**< CPU-side start address. */
	uint64_t pci_addr; /**< PCI-side start address. */
	uint64_t size; /**< Size of the window, in bytes. */
};

/**
 * @struct pcie_controller_ops
 * @brief Operations implemented by the SoC-specific PCIe controller driver.
 */
struct pcie_controller_ops {
	int (*init)(struct pcie_controller *controller); /**< Initialize the controller hardware. */
	void (*exit)(struct pcie_controller *controller); /**< Shut down the controller hardware. */
	int (*read_dbi)(struct pcie_controller *controller, uint32_t offset,
			uint8_t size, uint32_t *value); /**< Read from DBI space. */
	int (*write_dbi)(struct pcie_controller *controller, uint32_t offset,
			uint8_t size, uint32_t value); /**< Write to DBI space. */
	int (*dbi_ro_write_enable)(struct pcie_controller *controller,
			bool enable); /**< Enable or disable DBI read-only register writes. */
	int (*set_ep_bar)(struct pcie_controller *controller, uint8_t function,
			uint8_t bar, bool enable, bool bar_64bit); /**< Configure an endpoint BAR. */
	int (*read_app)(struct pcie_controller *controller, uint32_t offset,
			uint32_t *value); /**< Read from application registers. */
	int (*write_app)(struct pcie_controller *controller, uint32_t offset,
			uint32_t value); /**< Write to application registers. */
	int (*set_mode)(struct pcie_controller *controller, enum pcie_mode mode); /**< Set the controller operating mode. */
	int (*set_link)(struct pcie_controller *controller); /**< Configure the link width and speed. */
	int (*change_speed)(struct pcie_controller *controller, uint8_t link_gen); /**< Change the link speed. */
	int (*ltssm)(struct pcie_controller *controller, bool enable); /**< Enable or disable the LTSSM. */
	bool (*link_up)(struct pcie_controller *controller); /**< Report whether the link is up. */
	int (*wait_link)(struct pcie_controller *controller, uint32_t timeout_us); /**< Wait for the link to come up. */
	int (*program_atu)(struct pcie_controller *controller,
			const struct pcie_atu_region *region); /**< Program an ATU window. */
	int (*disable_atu)(struct pcie_controller *controller,
			enum pcie_atu_direction direction, uint8_t index); /**< Disable an ATU window. */
};

/**
 * @struct pcie_controller
 * @brief Runtime PCIe controller instance.
 */
struct pcie_controller {
	struct pcie_controller_config config; /**< Controller hardware configuration. */
	const struct pcie_controller_ops *ops; /**< SoC-specific controller operations. */
	bool initialized; /**< Whether the controller has been initialized. */
};

/**
 * @brief Initialize the PCIe controller using the built-in operations.
 */
int pcie_controller_init(struct pcie_controller *controller,
		const struct pcie_controller_config *config);

/**
 * @brief Initialize the PCIe controller with a caller-supplied operations table.
 */
int pcie_controller_init_with_ops(struct pcie_controller *controller,
		const struct pcie_controller_config *config,
		const struct pcie_controller_ops *ops);

/**
 * @brief Shut down the PCIe controller.
 */
void pcie_controller_exit(struct pcie_controller *controller);

/**
 * @brief Read a value from DBI register space.
 */
int pcie_controller_dbi_read(struct pcie_controller *controller,
		uint32_t offset, uint8_t size, uint32_t *value);

/**
 * @brief Write a value to DBI register space.
 */
int pcie_controller_dbi_write(struct pcie_controller *controller,
		uint32_t offset, uint8_t size, uint32_t value);

/**
 * @brief Read a value from application register space.
 */
int pcie_controller_app_read(struct pcie_controller *controller,
		uint32_t offset, uint32_t *value);

/**
 * @brief Write a value to application register space.
 */
int pcie_controller_app_write(struct pcie_controller *controller,
		uint32_t offset, uint32_t value);

/**
 * @brief Enable or disable writes to DBI read-only registers.
 */
int pcie_controller_dbi_ro_write_enable(struct pcie_controller *controller,
		bool enable);

/**
 * @brief Configure an endpoint BAR in the controller.
 */
int pcie_controller_set_ep_bar(struct pcie_controller *controller,
		uint8_t function, uint8_t bar, bool enable, bool bar_64bit);

/**
 * @brief Set the controller operating mode.
 */
int pcie_controller_set_mode(struct pcie_controller *controller,
		enum pcie_mode mode);

/**
 * @brief Configure the link width and speed.
 */
int pcie_controller_set_link(struct pcie_controller *controller);

/**
 * @brief Change the PCIe link speed.
 */
int pcie_controller_change_speed(struct pcie_controller *controller,
		uint8_t link_gen);

/**
 * @brief Enable or disable the LTSSM.
 */
int pcie_controller_ltssm(struct pcie_controller *controller, bool enable);

/**
 * @brief Check whether the PCIe link is up.
 */
bool pcie_controller_link_up(struct pcie_controller *controller);

/**
 * @brief Wait for the PCIe link to come up.
 */
int pcie_controller_wait_link(struct pcie_controller *controller,
		uint32_t timeout_us);

/**
 * @brief Program an ATU translation window.
 */
int pcie_controller_program_atu(struct pcie_controller *controller,
		const struct pcie_atu_region *region);

/**
 * @brief Disable an ATU translation window.
 */
int pcie_controller_disable_atu(struct pcie_controller *controller,
		enum pcie_atu_direction direction, uint8_t index);

/**
 * @brief Read a value from PCI configuration space.
 */
int pcie_controller_cfg_read(struct pcie_controller *controller,
		uint32_t bdf, uint32_t offset, uint8_t size, uint32_t *value);

/**
 * @brief Write a value to PCI configuration space.
 */
int pcie_controller_cfg_write(struct pcie_controller *controller,
		uint32_t bdf, uint32_t offset, uint8_t size, uint32_t value);

/**
 * @brief Find a PCI capability in configuration space.
 */
int pcie_controller_find_capability(struct pcie_controller *controller,
		uint32_t function_offset, uint8_t capability);

/**
 * @brief Find a PCIe extended capability in configuration space.
 */
int pcie_controller_find_ext_capability(struct pcie_controller *controller,
		uint32_t function_offset, uint16_t capability);

#endif /* __DRIVERS_PCIE_CONTROLLER_H__ */
