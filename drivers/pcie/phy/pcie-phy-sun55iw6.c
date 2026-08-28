/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stdint.h>

#include <io.h>
#include <log.h>

#include <drivers/pcie/phy/pcie-phy.h>

#if defined(CONFIG_SOC_SUN55IW6)

#define SUN55IW6_CCU_PCIE_AUX             0x1380U
#define SUN55IW6_CCU_PCIE_SLV             0x1384U
#define SUN55IW6_CCU_PCIE_BGR             0x138cU
#define SUN55IW6_CCU_PHY_CFG              0x13c0U
#define SUN55IW6_CCU_PHY_REF              0x13c4U
#define SUN55IW6_CCU_SERDES_BGR           0x13ccU
#define SUN55IW6_CCU_SERDES_AXI           0x13e0U
#define SUN55IW6_CCU_ITS0_BGR             0x0574U

#define SUN55IW6_PCK600_PWR_CTRL          0x07063000U
#define SUN55IW6_SERDES_PCIE_BGR          0x0004U
#define SUN55IW6_SERDES_PHY_CTL           0x0010U

#define SUBSYS_PHY_USE_SEL                (1U << 31)
#define SUBSYS_PHY_REF_CLK_SEL            (1U << 30)
#define SUBSYS_PHY_PIPE_SW               (1U << 9)
#define SUBSYS_PHY_PIPE_SEL              (1U << 8)
#define SUBSYS_PHY_FPGA_SYS_RSTN         (1U << 1)
#define SUBSYS_PHY_RSTN                  (1U << 0)

#define SUN55IW6_SERDES_PCIE_CLK_MASK     ((1U << 18) | (1U << 17) | \
							(1U << 16) | (1U << 1) | (1U << 0))
#define SUN55IW6_SERDES_CCU_RESET_MASK    ((1U << 17) | (1U << 16))
#define SUN55IW6_ITS0_ENABLE_MASK         ((1U << 16) | (1U << 1))
#define SUN55IW6_PCIE_CLK_ENABLE          (1U << 31)

static void pcie_phy_update(uintptr_t address, uint32_t clear, uint32_t set)
{
	uint32_t value = readl(address);

	value &= ~clear;
	value |= set;
	writel(value, address);
}

static void sun55iw6_configure_clocks(const struct pcie_phy *phy, bool enable)
{
	uintptr_t ccu = phy->config.ccu_base;
	uintptr_t subsys = phy->config.subsys_base;
	if (enable) {
		pcie_phy_update(ccu + SUN55IW6_CCU_SERDES_BGR, 0U,
			SUN55IW6_SERDES_CCU_RESET_MASK);
		pcie_phy_update(ccu + SUN55IW6_CCU_PCIE_BGR, 0U,
			(1U << 17) | (1U << 16));
		pcie_phy_update(ccu + SUN55IW6_CCU_PHY_REF,
			(1U << 31) | (1U << 24) | 0x1fU,
			(1U << 31) | (1U << 24) | 5U);
		pcie_phy_update(ccu + SUN55IW6_CCU_PHY_CFG,
			(1U << 31) | (1U << 24) | 0x1fU,
			(1U << 31) | (1U << 24) | 1U);
		pcie_phy_update(ccu + SUN55IW6_CCU_SERDES_AXI,
			(1U << 31) | (0x3U << 24) | 0x1fU,
			(1U << 31) | (0x1U << 24));
		pcie_phy_update(ccu + SUN55IW6_CCU_SERDES_BGR, 0U,
			(1U << 1) | (1U << 0));
		/* AUX: 24 MHz HOSC; SLV: 300 MHz peri0 PLL, both undivided. */
		pcie_phy_update(ccu + SUN55IW6_CCU_PCIE_AUX,
			(1U << 31) | (1U << 24) | 0x1fU,
			SUN55IW6_PCIE_CLK_ENABLE);
		pcie_phy_update(ccu + SUN55IW6_CCU_PCIE_SLV,
			(1U << 31) | (1U << 24) | 0x1fU,
			SUN55IW6_PCIE_CLK_ENABLE);
		pcie_phy_update(ccu + SUN55IW6_CCU_PCIE_BGR, 0U,
			(1U << 17) | (1U << 16));
		pcie_phy_update(ccu + SUN55IW6_CCU_ITS0_BGR, 0U,
			SUN55IW6_ITS0_ENABLE_MASK);
		pcie_phy_update(subsys + SUN55IW6_SERDES_PCIE_BGR, 0U,
			SUN55IW6_SERDES_PCIE_CLK_MASK);
		return;
	}

	pcie_phy_update(subsys + SUN55IW6_SERDES_PCIE_BGR,
		SUN55IW6_SERDES_PCIE_CLK_MASK, 0U);
	pcie_phy_update(ccu + SUN55IW6_CCU_PCIE_AUX, 1U << 31, 0U);
	pcie_phy_update(ccu + SUN55IW6_CCU_PCIE_SLV, 1U << 31, 0U);
	pcie_phy_update(ccu + SUN55IW6_CCU_PHY_REF, 1U << 31, 0U);
	pcie_phy_update(ccu + SUN55IW6_CCU_PHY_CFG, 1U << 31, 0U);
	pcie_phy_update(ccu + SUN55IW6_CCU_SERDES_AXI, 1U << 31, 0U);
	pcie_phy_update(ccu + SUN55IW6_CCU_SERDES_BGR,
		(1U << 1) | (1U << 0), 0U);
	pcie_phy_update(ccu + SUN55IW6_CCU_PCIE_BGR,
		(1U << 17) | (1U << 16), 0U);
	pcie_phy_update(ccu + SUN55IW6_CCU_ITS0_BGR,
		SUN55IW6_ITS0_ENABLE_MASK, 0U);
	pcie_phy_update(ccu + SUN55IW6_CCU_SERDES_BGR,
		SUN55IW6_SERDES_CCU_RESET_MASK, 0U);
}

static void sun55iw6_subsys_phy_enable(const struct pcie_phy *phy)
{
	uintptr_t address = phy->config.subsys_base + SUN55IW6_SERDES_PHY_CTL;
	uint32_t value;

	/* Select PCIe and assert the subsystem PHY before releasing its resets. */
	value = readl(address);
	value &= ~(SUBSYS_PHY_USE_SEL | SUBSYS_PHY_PIPE_SW |
		SUBSYS_PHY_PIPE_SEL | SUBSYS_PHY_FPGA_SYS_RSTN | SUBSYS_PHY_RSTN);
	writel(value, address);

	value &= ~SUBSYS_PHY_REF_CLK_SEL;
	value |= SUBSYS_PHY_RSTN;
	writel(value, address);

	value |= SUBSYS_PHY_FPGA_SYS_RSTN;
	writel(value, address);
}

static void sun55iw6_subsys_phy_disable(const struct pcie_phy *phy)
{
	uintptr_t address = phy->config.subsys_base + SUN55IW6_SERDES_PHY_CTL;
	uint32_t value = readl(address);

	value &= ~(SUBSYS_PHY_USE_SEL | SUBSYS_PHY_PIPE_SW |
		SUBSYS_PHY_PIPE_SEL | SUBSYS_PHY_RSTN);
	writel(value, address);
}

/* This sequence is required by the sun55iw6 INNO PHY clock block. */
static void sun55iw6_phy_100m_setup(const struct pcie_phy *phy)
{
	uintptr_t base = phy->config.phy_base;

	pcie_phy_update(base + 0x1004U, (0x3U << 3) | (1U << 0),
		(1U << 0) | (1U << 2) | (1U << 4));
	pcie_phy_update(base + 0x1018U, 0x3U << 4, 0x3U << 4);
	pcie_phy_update(base + 0x101cU, 0x0fffffffU, 0U);
	pcie_phy_update(base + 0x107cU, 0x3ffffU, (0x2U << 12) | 0x32U);
	pcie_phy_update(base + 0x1030U, 0x3U << 20, 0U);
	pcie_phy_update(base + 0x1050U, 0x7U << 5, 0x1U << 5);
	pcie_phy_update(base + 0x1054U, 0x7U << 5, 0x1U << 5);
	pcie_phy_update(base + 0x0804U, 0xfU << 4, 0xcU << 4);
	pcie_phy_update(base + 0x109cU, 0x3U << 8, 0x1U << 1);
	writel(0x80540a0aU, base + 0x1418U);
}

static int sun55iw6_phy_init(struct pcie_phy *phy)
{
	if (phy == NULL || phy->config.subsys_base == 0U ||
	    phy->config.phy_base == 0U || phy->config.ccu_base == 0U)
		return PCIE_ERR_INVALID;

	/* PCK600 must be powered before accessing the SerDes subsystem. */
	writel(0x8U, SUN55IW6_PCK600_PWR_CTRL);
	sun55iw6_configure_clocks(phy, true);
	sun55iw6_phy_100m_setup(phy);
	sun55iw6_subsys_phy_enable(phy);
	return PCIE_OK;
}

static void sun55iw6_phy_exit(struct pcie_phy *phy)
{
	if (phy == NULL || !phy->initialized)
		return;
	sun55iw6_subsys_phy_disable(phy);
	sun55iw6_configure_clocks(phy, false);
}

const struct pcie_phy_ops pcie_phy_sun55iw6_ops = {
	.init = sun55iw6_phy_init,
	.exit = sun55iw6_phy_exit,
};

int pcie_phy_init(struct pcie_phy *phy, const struct pcie_phy_config *config)
{
	int ret;

	if (phy == NULL || config == NULL)
		return PCIE_ERR_INVALID;
	*phy = (struct pcie_phy){ 0 };
	phy->config = *config;
	phy->ops = &pcie_phy_sun55iw6_ops;
	ret = phy->ops->init(phy);
	if (ret)
		return ret;
	phy->initialized = true;
	return PCIE_OK;
}

void pcie_phy_exit(struct pcie_phy *phy)
{
	if (phy == NULL || !phy->initialized)
		return;
	if (phy->ops != NULL && phy->ops->exit != NULL)
		phy->ops->exit(phy);
	phy->initialized = false;
}

#endif /* CONFIG_SOC_SUN55IW6 */
