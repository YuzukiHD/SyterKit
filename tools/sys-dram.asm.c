
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void mctl_phy_ca_bit_delay_compensation(__dram_para_t *para)

{
	uint uVar1;
	uint chip_id;
	uint type;
	uint reg_val;
	uint i;

	if ((para->dram_tpr10 & 0x10000) != 0) {
		if ((REG32(0x03006200) & 0xffff) == 0x800) {
			switch (para->dram_type) {
				case 3:
					uVar1 = para->dram_tpr10;
					for (i = 0; i < 0x20; i = i + 1) { *(uint *) ((i + 0x120c1e0) * 4) = (uVar1 >> 4 & 0xf) << 1; }
					REG32(0x048307dc) = (para->dram_tpr10 & 0xf) << 1;
					REG32(0x048307e4) = (para->dram_tpr10 >> 8 & 0xf) << 1;
					REG32(0x048307e0) = REG32(0x048307dc);
					if ((para->dram_para2 & 0x1000) != 0) {
						REG32(0x04832388) = (para->dram_tpr10 >> 0xc & 0xf) << 1;
					}
					break;
				case 4:
				case 7:
					break;
				case 8:
					uVar1 = para->dram_tpr10;
					for (i = 0; i < 0x20; i = i + 1) { *(uint *) ((i + 0x120c1e0) * 4) = (uVar1 >> 4 & 0xf) << 1; }
					REG32(0x048307dc) = (para->dram_tpr10 & 0xf) << 1;
					REG32(0x048307e4) = (para->dram_tpr10 >> 8 & 0xf) << 1;
					REG32(0x048307e0) = REG32(0x048307dc);
					if ((para->dram_para2 & 0x1000) != 0) {
						REG32(0x04830790) = (para->dram_tpr10 >> 0xc & 0xf) << 1;
					}
			}
		} else {
			switch (para->dram_type) {
				case 3:
					uVar1 = para->dram_tpr10;
					for (i = 0; i < 0x20; i = i + 1) { *(uint *) ((i + 0x120c1e0) * 4) = (uVar1 >> 4 & 0xf) << 1; }
					REG32(0x048307dc) = (para->dram_tpr10 & 0xf) << 1;
					REG32(0x048307b8) = (para->dram_tpr10 >> 8 & 0xf) << 1;
					REG32(0x048307e0) = REG32(0x048307dc);
					if ((para->dram_para2 & 0x1000) != 0) {
						REG32(0x04830784) = (para->dram_tpr10 >> 0xc & 0xf) << 1;
					}
					break;
				case 4:
					uVar1 = para->dram_tpr10;
					for (i = 0; i < 0x20; i = i + 1) { *(uint *) ((i + 0x120c1e0) * 4) = (uVar1 >> 4 & 0xf) << 1; }
					REG32(0x048307dc) = (para->dram_tpr10 & 0xf) << 1;
					REG32(0x04830784) = (para->dram_tpr10 >> 8 & 0xf) << 1;
					REG32(0x048307e0) = REG32(0x048307dc);
					break;
				case 7:
					uVar1 = para->dram_tpr10;
					for (i = 0; i < 0x20; i = i + 1) { *(uint *) ((i + 0x120c1e0) * 4) = (uVar1 >> 4 & 0xf) << 1; }
					REG32(0x048307dc) = (para->dram_tpr10 & 0xf) << 1;
					REG32(0x04830788) = (para->dram_tpr10 >> 8 & 0xf) << 1;
					REG32(0x048307e0) = REG32(0x048307dc);
					if ((para->dram_para2 & 0x1000) != 0) {
						REG32(0x04830790) = (para->dram_tpr10 >> 0xc & 0xf) << 1;
					}
					break;
				case 8:
			}
		}
	}
	return;
}
