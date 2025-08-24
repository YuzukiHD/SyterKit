/* SPDX-License-Identifier: GPL-2.0+ */

#include <barrier.h>
#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <jmp.h>
#include <log.h>

#include <sys-dram.h>
#include <sys-rtc.h>

static uint32_t training_error_flag = 0;

/* defines */
uint32_t mctl_phy_init(dram_para_t *para);
uint32_t mctl_channel_init(dram_para_t *para);
void mctl_phy_cold_reset(void);
void mctl_com_set_controller_after_phy(dram_para_t *para);
void mctl_com_init(dram_para_t *para);
void mctl_com_set_bus_config(dram_para_t *para);
uint32_t mctl_core_init(dram_para_t *para);
void mctl_sys_init(dram_para_t *para);

uint32_t phy_write_training(dram_para_t *para) {
	uint32_t uVar1;
	bool bVar2;
	uint32_t reg_val3;
	uint32_t reg_val2;
	uint32_t reg_val1;
	uint32_t reg_val;
	uint32_t write_training_error;
	uint32_t i;

	REG32(0x04830134) = 0;
	REG32(0x04830138) = 0;
	REG32(0x0483019c) = 0;
	REG32(0x048301a0) = 0;
	REG32(0x04830198) = REG32(0x04830198) & 0xfffffff3 | 8;
	REG32(0x04830190) = REG32(0x04830190) | 0x30;
	do {
	} while ((REG32(0x048308e0) & 3) != 3);
	bVar2 = (REG32(0x048308e0) & 0xc) != 0;
	if (bVar2) {
		printf("dx_low 16bit write training error  \n");
	}
	if ((para->dram_para2 & 1) == 0) {
		do {
		} while ((REG32(0x04830ae0) & 3) != 3);
		if ((REG32(0x04830ae0) & 0xc) != 0) {
			printf("dx_high 16bit write training error  \n");
			bVar2 = true;
		}
	}
	for (i = 0; i < 9; i = i + 1) {}
	for (i = 0; i < 9; i = i + 1) {
		uVar1 = *(int *) ((i + 0x120c24e) * 4) - *(int *) ((i + 0x120c23c) * 4);
		if ((uVar1 < 7) && (printf("write dx0_dq%d delay_width_error =0x%x \n", i - 1, uVar1), (para->dram_tpr10 & 0x10000000) == 0)) {
			bVar2 = true;
		}
	}
	for (i = 0; i < 9; i = i + 1) {}
	for (i = 0; i < 9; i = i + 1) {
		uVar1 = *(int *) ((i + 0x120c257) * 4) - *(int *) ((i + 0x120c245) * 4);
		if ((uVar1 < 7) && (printf("write dx1_dq%d delay_width_error =0x%x \n", i - 1, uVar1), (para->dram_tpr10 & 0x10000000) == 0)) {
			bVar2 = true;
		}
	}
	if ((para->dram_para2 & 1) == 0) {
		for (i = 0; i < 9; i = i + 1) {}
		for (i = 0; i < 9; i = i + 1) {
			uVar1 = *(int *) ((i + 0x120c2ce) * 4) - *(int *) ((i + 0x120c2bc) * 4);
			if ((uVar1 < 7) && (printf("write dx2_dq%d delay_width_error =0x%x \n", i - 1, uVar1), (para->dram_tpr10 & 0x10000000) == 0)) {
				bVar2 = true;
			}
		}
		for (i = 0; i < 9; i = i + 1) {}
		for (i = 0; i < 9; i = i + 1) {
			uVar1 = *(int *) ((i + 0x120c2d7) * 4) - *(int *) ((i + 0x120c2c5) * 4);
			if ((uVar1 < 7) && (printf("write dx3_dq%d delay_width_error =0x%x \n", i - 1, uVar1), (para->dram_tpr10 & 0x10000000) == 0)) {
				bVar2 = true;
			}
		}
	}
	REG32(0x04830190) = REG32(0x04830190) & 0xffffff9f;
	if ((para->dram_para2 & 0x1000) != 0) {
		REG32(0x04830198) = REG32(0x04830198) & 0xfffffff3 | 4;
		REG32(0x04830190) = REG32(0x04830190) | 0x30;
		do {
		} while ((REG32(0x048308e0) & 3) != 3);
		if ((REG32(0x048308e0) & 0xc) != 0) {
			printf("dx_low 16bit write training error  \n");
			bVar2 = true;
		}
		if ((para->dram_para2 & 1) == 0) {
			do {
			} while ((REG32(0x04830ae0) & 3) != 3);
			if ((REG32(0x04830ae0) & 0xc) != 0) {
				printf("dx_high 16bit write training error  \n");
				bVar2 = true;
			}
		}
		REG32(0x04830190) = REG32(0x04830190) & 0xffffff9f;
	}
	REG32(0x04830198) = REG32(0x04830198) & 0xfffffff3;
	return (uint32_t) !bVar2;
}

uint32_t phy_read_training(dram_para_t *para) {
	uint32_t uVar1;
	bool bVar2;
	uint32_t reg_val3;
	uint32_t reg_val2;
	uint32_t reg_val1;
	uint32_t reg_val;
	uint32_t read_training_error;
	uint32_t i;
	uint32_t dqs_read_default_deskew;

	if (para->dram_type == 8) {
		REG32(0x04830800) = 0;
		REG32(0x0483081c) = 0;
	}
	uVar1 = para->dram_para1 >> 0x10 & 0xf;
	dqs_read_default_deskew = uVar1 << 1;
	if (uVar1 == 0) {
		dqs_read_default_deskew = 0xf;
	}
	REG32(0x04830198) = REG32(0x04830198) & 0xfffffffc | 2;
	REG32(0x04830804) = dqs_read_default_deskew | REG32(0x04830804) & 0xffffffc0;
	REG32(0x04830808) = dqs_read_default_deskew | REG32(0x04830808) & 0xffffffc0;
	REG32(0x04830a04) = dqs_read_default_deskew | REG32(0x04830a04) & 0xffffffc0;
	REG32(0x04830a08) = dqs_read_default_deskew | REG32(0x04830a08) & 0xffffffc0;
	REG32(0x04830190) = REG32(0x04830190) | 7;
	do {
	} while ((REG32(0x04830840) & 0xc) != 0xc);
	bVar2 = (REG32(0x04830840) & 3) != 0;
	if (bVar2) {
		printf("dx_low 16bit read training error  \n");
	}
	if ((para->dram_para2 & 1) == 0) {
		do {
		} while ((REG32(0x04830a40) & 0xc) != 0xc);
		if ((REG32(0x04830a40) & 3) != 0) {
			printf("dx_high 16bit read training error  \n");
			bVar2 = true;
		}
	}
	for (i = 0; i < 9; i = i + 1) {}
	for (i = 0; i < 9; i = i + 1) {
		uVar1 = *(int *) ((i + 0x120c226) * 4) - *(int *) ((i + 0x120c214) * 4);
		if ((uVar1 < 7) && (printf("read dx0_dq%d delay_width_error =0x%x \n", i - 1, uVar1), (para->dram_tpr10 & 0x10000000) == 0)) {
			bVar2 = true;
		}
	}
	for (i = 0; i < 9; i = i + 1) {}
	for (i = 0; i < 9; i = i + 1) {
		uVar1 = *(int *) ((i + 0x120c22f) * 4) - *(int *) ((i + 0x120c21d) * 4);
		if ((uVar1 < 7) && (printf("read dx1_dq%d delay_width_error =0x%x \n", i - 1, uVar1), (para->dram_tpr10 & 0x10000000) == 0)) {
			bVar2 = true;
		}
	}
	if ((para->dram_para2 & 1) == 0) {
		for (i = 0; i < 9; i = i + 1) {}
		for (i = 0; i < 9; i = i + 1) {
			uVar1 = *(int *) ((i + 0x120c2a6) * 4) - *(int *) ((i + 0x120c294) * 4);
			if ((uVar1 < 7) && (printf("read dx2_dq%d delay_width_error =0x%x \n", i - 1, uVar1), (para->dram_tpr10 & 0x10000000) == 0)) {
				bVar2 = true;
			}
		}
		for (i = 0; i < 9; i = i + 1) {}
		for (i = 0; i < 9; i = i + 1) {
			uVar1 = *(int *) ((i + 0x120c2af) * 4) - *(int *) ((i + 0x120c29d) * 4);
			if ((uVar1 < 7) && (printf("read dx3_dq%d delay_width_error =0x%x \n", i - 1, uVar1), (para->dram_tpr10 & 0x10000000) == 0)) {
				bVar2 = true;
			}
		}
	}
	REG32(0x04830190) = REG32(0x04830190) & 0xfffffffc;
	if ((para->dram_para2 & 0x1000) != 0) {
		REG32(0x04830198) = REG32(0x04830198) & 0xfffffffc | 2;
		REG32(0x04830190) = REG32(0x04830190) | 7;
		do {
		} while ((REG32(0x04830840) & 0xc) != 0xc);
		if ((REG32(0x04830840) & 3) != 0) {
			printf("dx_low 16bit read training error  \n");
			bVar2 = true;
		}
		if ((para->dram_para2 & 1) == 0) {
			do {
			} while ((REG32(0x04830a40) & 0xc) != 0xc);
			if ((REG32(0x04830a40) & 3) != 0) {
				printf("dx_high 16bit read training error  \n");
				bVar2 = true;
			}
		}
		REG32(0x04830190) = REG32(0x04830190) & 0xfffffffc;
	}
	REG32(0x04830198) = REG32(0x04830198) & 0xfffffffc;
	return (uint32_t) !bVar2;
}

uint32_t phy_read_calibration(dram_para_t *para) {
	bool bVar1;
	uint32_t reg_val;
	uint32_t read_calibration_error;

	bVar1 = false;
	if (para->dram_type == 4) {
		REG32(0x04830054) = REG32(0x04830054) | 2;
	}
	if ((para->dram_para2 & 1) == 0) {
		do {
			if ((REG32(0x04830184) & 0xf) == 0xf)
				goto LAB_0001231a;
		} while ((REG32(0x04830184) >> 5 & 1) == 0);
		bVar1 = true;
	} else {
		do {
			if ((REG32(0x04830184) & 3) == 3)
				goto LAB_0001231a;
		} while ((REG32(0x04830184) >> 5 & 1) == 0);
		bVar1 = true;
	}
LAB_0001231a:
	if ((para->dram_para2 & 0x1000) != 0) {
		if ((para->dram_para2 & 1) == 0) {
			do {
				if ((REG32(0x04830184) & 0xf) == 0xf)
					goto LAB_00012422;
			} while ((REG32(0x04830184) >> 5 & 1) == 0);
			bVar1 = true;
		} else {
			do {
				if ((REG32(0x04830184) & 3) == 3)
					goto LAB_00012422;
			} while ((REG32(0x04830184) >> 5 & 1) == 0);
			bVar1 = true;
		}
	}
LAB_00012422:
	REG32(0x04830008) = REG32(0x04830008) & 0xffffffce;
	return (uint32_t) !bVar1;
}

uint32_t phy_write_leveling(dram_para_t *para) {
	bool bVar1;
	uint32_t uVar2;
	int iVar3;
	uint32_t type;
	uint32_t reg_val;
	uint32_t write_leveling_error;
	uint32_t j;
	uint32_t i;

	bVar1 = false;
	uVar2 = para->dram_type;
	if (uVar2 == 4) {
		REG32(0x0483000c) = para->dram_mr1 & 0xff;
		REG32(0x04830010) = para->dram_mr1 >> 8 & 0xff | 0x40;
	} else if (((uVar2 == 6) || (uVar2 == 7)) || (uVar2 == 8)) {
		REG32(0x0483000c) = para->dram_mr2 & 0xff;
		REG32(0x04830010) = para->dram_mr2 >> 8 & 0xff;
	} else {
		REG32(0x0483000c) = 4;
		REG32(0x04830010) = 0x40;
	}
	if ((para->dram_para2 & 1) == 0) {
		do {
		} while ((REG32(0x04830188) & 0xf) != 0xf);
	} else {
		do { } while ((REG32(0x04830188) & 3) != 3); }
	for (i = 0; i < 4; i = i + 1) {
		if (i < 2) {
			j = i;
		} else {
			j = i + 0x2e;
		}
		iVar3 = *(int *) ((j + 0x120c096) * 4);
		if ((iVar3 == 0) || (iVar3 == 0x3f)) {
			bVar1 = true;
		}
	}
	if ((para->dram_para2 & 0x1000) != 0) {
		if ((para->dram_para2 & 1) == 0) {
			do {
			} while ((REG32(0x04830188) & 0xf) != 0xf);
		} else {
			do { } while ((REG32(0x04830188) & 3) != 3); }
	}
	REG32(0x04830008) = REG32(0x04830008) & 0xffffff3b;
	return (uint32_t) !bVar1;
}

void mctl_phy_set_address_remapping(dram_para_t *para) {
	uint32_t remap_lpddr3_A100[27];
	uint32_t remap_ddr4_A100[27];
	uint32_t remap_ddr3_A100[27];
	uint32_t chip_id;
	uint32_t i;

	// memcpy(remap_ddr3_A100, &.LC0, 0x6c);
	// memcpy(remap_ddr4_A100, &.LC1, 0x6c);
	// memcpy(remap_lpddr3_A100, &.LC2, 0x6c);
	if ((REG32(0x03006200) & 0xffff) != 0x800) {
		if ((REG32(0x03006200) & 0xffff) == 0x1400) {
			printf("DRAM remap error\n");
		} else {
			switch (para->dram_type) {
				case 3:
					for (i = 0; i < 0x1b; i = i + 1) { *(uint32_t *) ((i + 0x120c030) * 4) = remap_ddr3_A100[i]; }
					break;
				case 4:
					for (i = 0; i < 0x1b; i = i + 1) { *(uint32_t *) ((i + 0x120c030) * 4) = remap_ddr4_A100[i]; }
					break;
				case 7:
					for (i = 0; i < 0x1b; i = i + 1) { *(uint32_t *) ((i + 0x120c030) * 4) = remap_lpddr3_A100[i]; }
					break;
				case 8:
					for (i = 0; i < 0x1b; i = i + 1) {}
			}
		}
	}
	return;
}

void mctl_phy_ca_bit_delay_compensation(dram_para_t *para) {
	uint32_t uVar1;
	uint32_t chip_id;
	uint32_t type;
	uint32_t reg_val;
	uint32_t i;

	if ((para->dram_tpr10 & 0x10000) != 0) {
		if ((REG32(0x03006200) & 0xffff) == 0x800) {
			switch (para->dram_type) {
				case 3:
					uVar1 = para->dram_tpr10;
					for (i = 0; i < 0x20; i = i + 1) { *(uint32_t *) ((i + 0x120c1e0) * 4) = (uVar1 >> 4 & 0xf) << 1; }
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
					for (i = 0; i < 0x20; i = i + 1) { *(uint32_t *) ((i + 0x120c1e0) * 4) = (uVar1 >> 4 & 0xf) << 1; }
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
					for (i = 0; i < 0x20; i = i + 1) { *(uint32_t *) ((i + 0x120c1e0) * 4) = (uVar1 >> 4 & 0xf) << 1; }
					REG32(0x048307dc) = (para->dram_tpr10 & 0xf) << 1;
					REG32(0x048307b8) = (para->dram_tpr10 >> 8 & 0xf) << 1;
					REG32(0x048307e0) = REG32(0x048307dc);
					if ((para->dram_para2 & 0x1000) != 0) {
						REG32(0x04830784) = (para->dram_tpr10 >> 0xc & 0xf) << 1;
					}
					break;
				case 4:
					uVar1 = para->dram_tpr10;
					for (i = 0; i < 0x20; i = i + 1) { *(uint32_t *) ((i + 0x120c1e0) * 4) = (uVar1 >> 4 & 0xf) << 1; }
					REG32(0x048307dc) = (para->dram_tpr10 & 0xf) << 1;
					REG32(0x04830784) = (para->dram_tpr10 >> 8 & 0xf) << 1;
					REG32(0x048307e0) = REG32(0x048307dc);
					break;
				case 7:
					uVar1 = para->dram_tpr10;
					for (i = 0; i < 0x20; i = i + 1) { *(uint32_t *) ((i + 0x120c1e0) * 4) = (uVar1 >> 4 & 0xf) << 1; }
					REG32(0x048307dc) = (para->dram_tpr10 & 0xf) << 1;
					REG32(0x04830788) = (para->dram_tpr10 >> 8 & 0xf) << 1;
					REG32(0x048307e0) = REG32(0x048307dc);
					if ((para->dram_para2 & 0x1000) != 0) {
						REG32(0x04830790) = (para->dram_tpr10 >> 0xc & 0xf) << 1;
					}
					break;
				case 8:
					break;
			}
		}
	}
	return;
}

void mctl_drive_odt_config(dram_para_t *para) {
	uint32_t reg_val;

	REG32(0x0483038c) = para->dram_dx_dri & 0x1f;
	REG32(0x04830388) = REG32(0x0483038c);
	if (para->dram_type == 8) {
		REG32(0x0483038c) = 4;
	}
	REG32(0x048303cc) = para->dram_dx_dri >> 8 & 0x1f;
	REG32(0x048303c8) = REG32(0x048303cc);
	if (para->dram_type == 8) {
		REG32(0x048303cc) = 4;
	}
	REG32(0x0483040c) = para->dram_dx_dri >> 0x10 & 0x1f;
	REG32(0x04830408) = REG32(0x0483040c);
	if (para->dram_type == 8) {
		REG32(0x0483040c) = 4;
	}
	REG32(0x0483044c) = para->dram_dx_dri >> 0x18 & 0x1f;
	REG32(0x04830448) = REG32(0x0483044c);
	if (para->dram_type == 8) {
		REG32(0x0483044c) = 4;
	}
	REG32(0x04830340) = para->dram_ca_dri & 0x1f;
	REG32(0x04830344) = REG32(0x04830340);
	REG32(0x04830348) = para->dram_ca_dri >> 8 & 0x1f;
	REG32(0x0483034c) = REG32(0x04830348);
	REG32(0x04830384) = para->dram_dx_odt & 0x1f;
	if ((para->dram_type == 4) || (REG32(0x04830380) = REG32(0x04830384), para->dram_type == 7)) {
		REG32(0x04830380) = 0;
	}
	if (para->dram_type == 8) {
		REG32(0x04830384) = 0;
	}
	REG32(0x048303c4) = para->dram_dx_odt >> 8 & 0x1f;
	if ((para->dram_type == 4) || (REG32(0x048303c0) = REG32(0x048303c4), para->dram_type == 7)) {
		REG32(0x048303c0) = 0;
	}
	if (para->dram_type == 8) {
		REG32(0x048303c4) = 0;
	}
	REG32(0x04830404) = para->dram_dx_odt >> 0x10 & 0x1f;
	if ((para->dram_type == 4) || (REG32(0x04830400) = REG32(0x04830404), para->dram_type == 7)) {
		REG32(0x04830400) = 0;
	}
	if (para->dram_type == 8) {
		REG32(0x04830404) = 0;
	}
	REG32(0x04830444) = para->dram_dx_odt >> 0x18 & 0x1f;
	if ((para->dram_type == 4) || (REG32(0x04830440) = REG32(0x04830444), para->dram_type == 7)) {
		REG32(0x04830440) = 0;
	}
	if (para->dram_type == 8) {
		REG32(0x04830444) = 0;
	}
	return;
}

void mctl_phy_vref_config(dram_para_t *para) {
	uint32_t reg_val;

	reg_val = 0;
	switch (para->dram_type) {
		case 3:
			if ((para->dram_tpr6 & 0xff) == 0) {
				reg_val = 0x80;
			} else {
				reg_val = para->dram_tpr6 & 0xff;
			}
			break;
		case 4:
			if ((para->dram_tpr6 >> 8 & 0xff) == 0) {
				reg_val = 0x80;
			} else {
				reg_val = para->dram_tpr6 >> 8 & 0xff;
			}
			break;
		case 7:
			if ((para->dram_tpr6 >> 0x10 & 0xff) == 0) {
				reg_val = 0x80;
			} else {
				reg_val = para->dram_tpr6 >> 0x10 & 0xff;
			}
			break;
		case 8:
			if (para->dram_tpr6 >> 0x18 == 0) {
				reg_val = 0x33;
			} else {
				reg_val = para->dram_tpr6 >> 0x18;
			}
	}
	REG32(0x048303dc) = reg_val;
	REG32(0x0483045c) = reg_val;
	return;
}

void phy_para_config(dram_para_t *para) {
	uint32_t uVar1;
	uint32_t type;
	uint32_t ctrl_freq;
	uint32_t reg_val;
	uint32_t TCWL;
	uint32_t TCL;

	uVar1 = para->dram_type;
	if (uVar1 == 8) {
		REG32(0x04830004) = REG32(0x04830004) & 0xffffff7f;
	}
	if ((para->dram_para2 & 1) == 0) {
		reg_val = REG32(0x0483003c) & 0xfffffff0 | 0xf;
	} else {
		reg_val = REG32(0x0483003c) & 0xfffffff0 | 3;
	}
	REG32(0x0483003c) = reg_val;
	switch (uVar1) {
		case 3:
			TCL = 0xd;
			TCWL = 9;
			break;
		case 4:
			TCL = 0xd;
			TCWL = 10;
			break;
		default:
			TCL = 0xd;
			TCWL = 9;
			break;
		case 7:
			TCL = 0xe;
			TCWL = 8;
			break;
		case 8:
			TCL = 0x14;
			TCWL = 10;
	}
	REG32(0x04830014) = TCL;
	REG32(0x0483035c) = TCL;
	REG32(0x04830368) = TCL;
	REG32(0x04830374) = TCL;
	REG32(0x04830018) = 0;
	REG32(0x04830360) = 0;
	REG32(0x0483036c) = 0;
	REG32(0x04830378) = 0;
	REG32(0x0483001c) = TCWL;
	REG32(0x04830364) = TCWL;
	REG32(0x04830370) = TCWL;
	REG32(0x0483037c) = TCWL;
	mctl_phy_set_address_remapping(para);
	mctl_phy_ca_bit_delay_compensation(para);
	mctl_phy_vref_config(para);
	mctl_drive_odt_config(para);
	REG32(0x04830004) = REG32(0x04830004) & 0xfffffff8;
	switch (uVar1) {
		case 3:
			reg_val = REG32(0x04830004) | 2;
			break;
		case 4:
			reg_val = REG32(0x04830004) | 4;
			break;
		default:
			reg_val = REG32(0x04830004) | 2;
			break;
		case 7:
			reg_val = REG32(0x04830004) | 3;
			break;
		case 8:
			reg_val = REG32(0x04830004) | 5;
	}
	REG32(0x04830004) = reg_val | 8;
	if (para->dram_clk < 0x2a1) {
		REG32(0x04830020) = 0xf;
	}
	if (para->dram_clk < 0x1f5) {
		REG32(0x04830144) = REG32(0x04830144) | 0x80;
		REG32(0x0483014c) = REG32(0x0483014c) & 0xffffff1f | 0x20;
	} else {
		REG32(0x04830144) = REG32(0x04830144) & 0xffffff7f;
		REG32(0x0483014c) = REG32(0x0483014c) & 0xffffff1f;
	}
	REG32(0x0483014c) = REG32(0x0483014c) & 0xfffffff7;
	do {
	} while ((REG32(0x04830180) >> 2 & 1) != 1);
	udelay(1000);
	REG32(0x04830058) = 0x37;
	REG32(0x04810008) = REG32(0x04810008) & 0xfffffdff;
	udelay(1);
	return;
}

void mctl_phy_dx_bit_delay_compensation(dram_para_t *para) {
	uint32_t uVar1;
	uint32_t reg_val;
	uint32_t i;

	if ((para->dram_tpr10 & 0x40000) != 0) {
		REG32(0x04830060) = REG32(0x04830060) & 0xfffffffe;
		REG32(0x04830008) = REG32(0x04830008) | 8;
		REG32(0x04830190) = REG32(0x04830190) & 0xffffffef;
		if (para->dram_type == 8) {
			REG32(0x04830004) = REG32(0x04830004) & 0xffffff7f;
		}
		uVar1 = para->dram_tpr11 & 0x3f;
		for (i = 0; i < 9; i = i + 1) {
			*(uint32_t *) (i * 8 + 0x4830484) = uVar1;
			*(uint32_t *) (i * 8 + 0x4830544) = uVar1;
		}
		REG32(0x048304cc) = para->dram_para0 & 0x3f;
		uVar1 = para->dram_tpr11 >> 8 & 0x3f;
		REG32(0x048304d0) = REG32(0x048304cc);
		REG32(0x0483058c) = REG32(0x048304cc);
		REG32(0x04830590) = REG32(0x048304cc);
		for (i = 0; i < 9; i = i + 1) {
			*(uint32_t *) ((i + 0x90609b) * 8) = uVar1;
			*(uint32_t *) ((i + 0x9060b3) * 8) = uVar1;
		}
		REG32(0x04830520) = para->dram_para0 >> 8 & 0x3f;
		uVar1 = para->dram_tpr11 >> 0x10 & 0x3f;
		REG32(0x04830524) = REG32(0x04830520);
		REG32(0x048305e0) = REG32(0x04830520);
		REG32(0x048305e4) = REG32(0x04830520);
		for (i = 0; i < 9; i = i + 1) {
			*(uint32_t *) (i * 8 + 0x4830604) = uVar1;
			*(uint32_t *) (i * 8 + 0x48306c4) = uVar1;
		}
		REG32(0x0483064c) = para->dram_para0 >> 0x10 & 0x3f;
		uVar1 = para->dram_tpr11 >> 0x18 & 0x3f;
		REG32(0x04830650) = REG32(0x0483064c);
		REG32(0x0483070c) = REG32(0x0483064c);
		REG32(0x04830710) = REG32(0x0483064c);
		for (i = 0; i < 9; i = i + 1) {
			*(uint32_t *) ((i + 0x9060cb) * 8) = uVar1;
			*(uint32_t *) ((i + 0x9060e3) * 8) = uVar1;
		}
		REG32(0x048306a0) = para->dram_para0 >> 0x18 & 0x3f;
		REG32(0x04830060) = REG32(0x04830060) | 1;
		REG32(0x048306a4) = REG32(0x048306a0);
		REG32(0x04830760) = REG32(0x048306a0);
		REG32(0x04830764) = REG32(0x048306a0);
	}
	if ((para->dram_tpr10 & 0x20000) != 0) {
		REG32(0x04830054) = REG32(0x04830054) & 0xffffff7f;
		REG32(0x04830190) = REG32(0x04830190) & 0xfffffffb;
		uVar1 = para->dram_tpr12 & 0x3f;
		for (i = 0; i < 9; i = i + 1) {
			*(uint32_t *) ((i + 0x906090) * 8) = uVar1;
			*(uint32_t *) ((i + 0x9060a8) * 8) = uVar1;
		}
		REG32(0x048304c8) = para->dram_tpr14 & 0x3f;
		uVar1 = para->dram_tpr12 >> 8 & 0x3f;
		REG32(0x04830528) = REG32(0x048304c8);
		REG32(0x04830588) = REG32(0x048304c8);
		REG32(0x048305e8) = REG32(0x048304c8);
		for (i = 0; i < 9; i = i + 1) {
			*(uint32_t *) (i * 8 + 0x48304d4) = uVar1;
			*(uint32_t *) (i * 8 + 0x4830594) = uVar1;
		}
		REG32(0x0483051c) = para->dram_tpr14 >> 8 & 0x3f;
		uVar1 = para->dram_tpr12 >> 0x10 & 0x3f;
		REG32(0x0483052c) = REG32(0x0483051c);
		REG32(0x048305dc) = REG32(0x0483051c);
		REG32(0x048305ec) = REG32(0x0483051c);
		for (i = 0; i < 9; i = i + 1) {
			*(uint32_t *) ((i + 0x9060c0) * 8) = uVar1;
			*(uint32_t *) ((i + 0x9060d8) * 8) = uVar1;
		}
		REG32(0x04830648) = para->dram_tpr14 >> 0x10 & 0x3f;
		uVar1 = para->dram_tpr12 >> 0x18 & 0x3f;
		REG32(0x048306a8) = REG32(0x04830648);
		REG32(0x04830708) = REG32(0x04830648);
		REG32(0x04830768) = REG32(0x04830648);
		for (i = 0; i < 9; i = i + 1) {
			*(uint32_t *) (i * 8 + 0x4830654) = uVar1;
			*(uint32_t *) (i * 8 + 0x4830714) = uVar1;
		}
		REG32(0x0483069c) = para->dram_tpr14 >> 0x18 & 0x3f;
		REG32(0x04830054) = REG32(0x04830054) | 0x80;
		REG32(0x048306ac) = REG32(0x0483069c);
		REG32(0x0483075c) = REG32(0x0483069c);
		REG32(0x0483076c) = REG32(0x0483069c);
	}
	return;
}

void mctl_dfi_init(dram_para_t *para) {
	uint32_t reg_val;

	do {
	} while ((REG32(0x04820324) & 1) != 1);
	do {
	} while ((REG32(0x048201bc) & 1) != 1);
	REG32(0x048201b0) = REG32(0x048201b0) & 0xffffffdf | 1;
	REG32(0x04820030) = REG32(0x04820030) & 0xffffffdf;
	REG32(0x04820320) = 1;
	do {
	} while ((REG32(0x04820324) & 1) != 1);
	do {
	} while ((REG32(0x04820004) & 3) != 1);
	udelay(200);
	REG32(0x048201b0) = REG32(0x048201b0) & 0xfffffffe;
	REG32(0x04820320) = 1;
	do {
	} while ((REG32(0x04820324) & 1) != 1);
	if (para->dram_type == 8) {
		do { } while (true); }
	if (para->dram_type == 7) {
		do { } while (true); }
	if (para->dram_type == 4) {
		do { } while (true); }
	if (para->dram_type != 3) {
		REG32(0x04830054) = 0;
		return;
	}
	do { } while (true); }

void mctl_com_set_controller_refresh(uint32_t onoff, dram_para_t *para) {
	uint32_t reg_val;

	REG32(0x04820060) = onoff | REG32(0x04820060) & 0xfffffffe;
	return;
}

uint32_t mctl_core_init(dram_para_t *para) {
	uint32_t uVar1;
	uint32_t ret_val;

	mctl_sys_init(para);
	uVar1 = mctl_channel_init(para);
	return uVar1;
}

uint32_t ddrphy_phyinit_C_initPhyConfig(dram_para_t *para) {
	bool bVar1;
	bool bVar2;
	bool bVar3;
	bool bVar4;
	uint32_t reg_val;
	uint32_t read_calibration_error;
	uint32_t write_training_error;
	uint32_t read_training_error;
	uint32_t write_leveling_error;
	uint32_t i;
	uint32_t ret_val;

	ret_val = 1;
	bVar2 = false;
	bVar3 = false;
	bVar4 = false;
	bVar1 = false;
	phy_para_config(para);
	mctl_dfi_init(para);
	REG32(0x04820320) = 0;
	mctl_com_set_controller_refresh(0, para);
	REG32(0x04820320) = 1;
	if ((para->dram_tpr10 & 0x100000) != 0) {
		if ((para->dram_tpr10 & 0x80000) == 0) {
			ret_val = phy_write_leveling(para);
		} else {
			for (i = 0; i < 5; i = i + 1) {
				ret_val = phy_write_leveling(para);
				if (ret_val == 1) {
					i = 5;
				}
			}
		}
		if (ret_val == 0) {
			printf("write_leveling error \n");
			bVar2 = true;
		}
	}
	if ((para->dram_tpr10 & 0x200000) != 0) {
		if ((para->dram_tpr10 & 0x80000) == 0) {
			ret_val = phy_read_calibration(para);
		} else {
			for (i = 0; i < 5; i = i + 1) {
				ret_val = phy_read_calibration(para);
				if (ret_val == 1) {
					i = 5;
				}
			}
		}
		if (ret_val == 0) {
			bVar1 = true;
		}
	}
	if (((para->dram_tpr10 & 0x400000) != 0) && (!bVar1)) {
		if ((para->dram_tpr10 & 0x80000) == 0) {
			ret_val = phy_read_training(para);
		} else {
			for (i = 0; i < 5; i = i + 1) {
				ret_val = phy_read_training(para);
				if (ret_val == 1) {
					i = 5;
				}
			}
		}
		if (ret_val == 0) {
			printf("read_training error \n");
			bVar3 = true;
		}
	}
	if (((para->dram_tpr10 & 0x800000) != 0) && (!bVar1)) {
		if ((para->dram_tpr10 & 0x80000) == 0) {
			ret_val = phy_write_training(para);
		} else {
			for (i = 0; i < 5; i = i + 1) {
				ret_val = phy_write_training(para);
				if (ret_val == 1) {
					i = 5;
				}
			}
		}
		if (ret_val == 0) {
			printf("write_training error \n");
			bVar4 = true;
		}
	}
	if ((para->dram_tpr10 & 0x80000) == 0) {
		if ((((bVar2) || (bVar1)) || (bVar3)) || (bVar4)) {
			bVar1 = true;
		} else {
			bVar1 = false;
		}
		if (bVar1) {
			return 0;
		}
	} else {
		if (((bVar2) || (bVar1)) || ((bVar3 || (bVar4)))) {
			bVar1 = true;
		} else {
			bVar1 = false;
		}
		if (bVar1) {
			training_error_flag = training_error_flag + 1;
			if (training_error_flag == 10) {
				printf("retraining ten \n");
				return 0;
			}
			ret_val = mctl_core_init(para);
			training_error_flag = 0;
		}
	}
	mctl_phy_dx_bit_delay_compensation(para);
	return ret_val;
}

uint32_t mctl_phy_init(dram_para_t *para) {
	uint32_t uVar1;
	uint32_t ret_val;

	mctl_phy_cold_reset();
	uVar1 = ddrphy_phyinit_C_initPhyConfig(para);
	return uVar1;
}

uint32_t mctl_channel_init(dram_para_t *para) {
	uint32_t uVar1;
	uint32_t reg_val;
	uint32_t ret_val;

	REG32(0x04810008) = REG32(0x04810008) & 0xfeffffff | 0x2000200;
	REG32(0x04810020) = REG32(0x04810020) | 0x8000;
	mctl_com_set_bus_config(para);
	REG32(0x04820038) = 0;
	mctl_com_init(para);
	uVar1 = mctl_phy_init(para);
	mctl_com_set_controller_after_phy(para);
	return uVar1;
}

void mctl_phy_cold_reset(void) {
	uint32_t reg_val;

	REG32(0x04810008) = REG32(0x04810008) & 0xfeffffff | 0x200;
	udelay(1);
	REG32(0x04810008) = REG32(0x04810008) | 0x1000000;
	udelay(1);
	return;
}

void phy_zq_calibration(dram_para_t *para) {
	uint32_t reg_val;

	do {
	} while ((REG32(0x048301ac) & 1) != 1);
	REG32(0x04830050) = REG32(0x04830050) & 0xffffffdf;
	REG32(0x04830394) = 0xff;
	REG32(0x048303d4) = 0xff;
	REG32(0x04830414) = 0xff;
	REG32(0x04830454) = 0xff;
	return;
}

void ccm_set_pll_ddr0_sscg(dram_para_t *para) {
	uint32_t reg_val;
	uint32_t ret;

	switch (para->dram_tpr13 >> 0x14 & 7) {
		case 0:
			break;
		case 1:
			REG32(0x03001110) = 0xe486cccc;
			break;
		case 2:
			REG32(0x03001110) = 0xe9069999;
			break;
		case 3:
			REG32(0x03001110) = 0xed866666;
			break;
		case 4:
			REG32(0x03001110) = 0xf2063333;
			break;
		case 5:
			REG32(0x03001110) = 0xf5860000;
			break;
		default:
			REG32(0x03001110) = 0xf2063333;
	}
	REG32(0x03001010) = REG32(0x03001010) | 0x1000000;
	return;
}

uint32_t _ccm_set_pll_ddr_clk(uint32_t pll_clk, dram_para_t *para) {
	uint32_t uVar1;
	uint32_t rval;
	uint32_t div;

	uVar1 = REG32(0x03001010) & 0xffff00fc | (pll_clk / 0x18 - 1) * 0x100;
	REG32(0x03001010) = uVar1 | 0x80000000;
	ccm_set_pll_ddr0_sscg(para);
	REG32(0x03001010) = uVar1 | 0xe0000000;
	REG32(0x03001010) = uVar1 | 0xa0000000;
	do {
	} while ((REG32(0x03001010) & 0x10000000) == 0);
	return (pll_clk / 0x18) * 0x18;
}

uint32_t DRAMC_get_dram_size(dram_para_t *para) {
	int iVar1;
	uint32_t uVar2;
	uint32_t temp;
	uint32_t dram_size;

	iVar1 = (para->dram_para2 >> 0xc & 0xf) + (para->dram_para1 >> 0xe & 3) + (para->dram_para1 >> 0xc & 3) + (para->dram_para1 >> 4 & 0xff) + (para->dram_para1 & 0xf);
	if ((para->dram_para2 & 0xf) == 0) {
		uVar2 = iVar1 - 0x12;
	} else {
		uVar2 = iVar1 - 0x13;
	}
	dram_size = 1 << (uVar2 & 0xff);
	return dram_size;
}

void Wait_time_config(uint32_t value) {
	uint32_t reg_val;

	REG32(0x04810004) = value << 8 | REG32(0x04810004) & 0xffff00ff;
	return;
}

uint32_t auto_scan_dram_rank_width(dram_para_t *para) {
	uint32_t uVar1;
	uint32_t uVar2;
	uint32_t uVar3;
	uint32_t uVar4;
	uint32_t uVar5;
	uint32_t temp_trp10;
	uint32_t temp_para2;
	uint32_t temp_para1;
	uint32_t temp_trp13;
	uint32_t ret_val;

	uVar2 = para->dram_tpr13;
	uVar3 = para->dram_para1;
	uVar4 = para->dram_para2;
	uVar5 = para->dram_tpr10;
	para->dram_tpr10 = para->dram_tpr10 | 0x10000000;
	para->dram_tpr13 = para->dram_tpr13 | 1;
	if (para->dram_type == 4) {
		para->dram_para1 = 0x60b7;
	} else {
		para->dram_para1 = 0x20b7;
	}
	para->dram_para2 = 0x1000;
	uVar1 = mctl_core_init(para);
	if (uVar1 == 0) {
		para->dram_para2 = 0;
		uVar1 = mctl_core_init(para);
		if (uVar1 == 0) {
			para->dram_para2 = 0x1001;
			uVar1 = mctl_core_init(para);
			if (uVar1 == 0) {
				para->dram_para2 = 1;
				uVar1 = mctl_core_init(para);
				if (uVar1 == 0) {
					return 0;
				}
				printf("[AUTO DEBUG]16 bit,1 ranks training success!\n");
			} else {
				printf("[AUTO DEBUG]16 bit,2 ranks training success!\n");
			}
		} else {
			printf("[AUTO DEBUG]32bit,1 ranks training success!\n");
		}
	} else {
		printf("[AUTO DEBUG]32bit,2 ranks training success!\n");
	}
	para->dram_tpr13 = uVar2;
	para->dram_para1 = uVar3;
	para->dram_para2 = para->dram_para2 | uVar4 & 0xffff0000;
	para->dram_tpr10 = uVar5;
	return 1;
}

uint32_t auto_scan_dram_size(dram_para_t *para) {
	uint32_t uVar1;
	uint32_t uVar2;
	int iVar3;
	uint32_t uVar4;
	uint32_t uVar5;
	uint32_t uVar6;
	uint32_t uVar7;
	uint32_t uVar8;
	uint32_t row_num;
	uint32_t bank_num;
	uint32_t col_num;
	uint32_t row_shift;
	uint32_t bank_shift;
	uint32_t col_shift;
	uint32_t temp_trp10;
	uint32_t rank_base_addr;
	uint32_t ret;
	uint32_t reg_val;
	uint32_t bg_num;
	uint32_t cnt;
	uint32_t j;
	uint32_t i;

	uVar4 = para->dram_tpr10;
	para->dram_tpr10 = para->dram_tpr10 | 0x10000000;
	if (para->dram_type == 4) {
		para->dram_para1 = 0xb0eb;
	} else {
		para->dram_para1 = 0x30eb;
	}
	if ((para->dram_para2 & 0xf) == 0) {
		iVar3 = 2;
	} else {
		iVar3 = 1;
	}
	iVar3 = (para->dram_para1 >> 0xe & 3) + iVar3;
	uVar5 = para->dram_para1;
	uVar1 = mctl_core_init(para);
	if (uVar1 == 0) {
		uVar4 = 0;
	} else {
		for (i = 0; i < 0x10; i = i + 1) {
			if ((i & 1) == 0) {
				uVar1 = ~(i * 4 + 0x40000000);
			} else {
				uVar1 = i * 4 + 0x40000000;
			}
			*(uint32_t *) (i * 4 + 0x40000000) = uVar1;
			dsb();
		}
		for (i = 1; i < 3; i = i + 1) {
			cnt = 0;
			for (j = 0; j < 0x10; j = j + 1) {
				if ((j & 1) == 0) {
					uVar1 = ~(j * 4 + 0x40000000);
				} else {
					uVar1 = j * 4 + 0x40000000;
				}
				if (uVar1 != *(uint32_t *) ((1 << (i + 5 & 0xff)) + 0x40000000 + j * 4))
					break;
				cnt = cnt + 1;
			}
			if (cnt == 0x10)
				break;
		}
		if (i == 1) {
			bg_num = 1;
		} else if (para->dram_type == 4) {
			bg_num = 2;
		} else {
			bg_num = 0;
		}
		for (i = 7; i < 0xb; i = i + 1) {
			cnt = 0;
			for (j = 0; j < 0x10; j = j + 1) {
				if ((j & 1) == 0) {
					uVar1 = ~(j * 4 + 0x40000000);
				} else {
					uVar1 = j * 4 + 0x40000000;
				}
				if (uVar1 != *(uint32_t *) ((1 << (iVar3 + i & 0xff)) + 0x40000000 + j * 4))
					break;
				cnt = cnt + 1;
			}
			if (cnt == 0x10)
				break;
		}
		if (10 < i) {
			i = 0xb;
		}
		uVar1 = i;
		for (i = 2; i < 3; i = i + 1) {
			cnt = 0;
			for (j = 0; j < 0x10; j = j + 1) {
				if ((j & 1) == 0) {
					uVar6 = ~(j * 4 + 0x40000000);
				} else {
					uVar6 = j * 4 + 0x40000000;
				}
				if (uVar6 != *(uint32_t *) ((1 << ((uVar5 & 0xf) + iVar3 + i & 0xff)) + 0x40000000 + j * 4))
					break;
				cnt = cnt + 1;
			}
			if (cnt == 0x10)
				break;
		}
		if (2 < i) {
			i = 3;
		}
		uVar5 = i;
		if (para->dram_type == 4) {
			para->dram_para1 = 0x6118;
		} else {
			para->dram_para1 = 0x2118;
		}
		uVar6 = para->dram_para1;
		if ((para->dram_para2 & 0xf) == 0) {
			iVar3 = 2;
		} else {
			iVar3 = 1;
		}
		uVar7 = para->dram_para1;
		uVar8 = para->dram_para1;
		uVar2 = mctl_core_init(para);
		if (uVar2 == 0) {
			uVar4 = 0;
		} else {
			for (i = 0; i < 0x10; i = i + 1) {
				if ((i & 1) == 0) {
					uVar2 = ~(i * 4 + 0x40000000);
				} else {
					uVar2 = i * 4 + 0x40000000;
				}
				*(uint32_t *) (i * 4 + 0x40000000) = uVar2;
				dsb();
			}
			for (i = 0xc; i < 0x11; i = i + 1) {
				cnt = 0;
				for (j = 0; j < 0x10; j = j + 1) {
					if ((j & 1) == 0) {
						uVar2 = ~(j * 4 + 0x40000000);
					} else {
						uVar2 = j * 4 + 0x40000000;
					}
					if (uVar2 != *(uint32_t *) ((1 << ((uVar8 >> 0xc & 3) + (uVar7 & 0xf) + (uVar6 >> 0xe & 3) + iVar3 + i & 0xff)) + 0x40000000 + j * 4))
						break;
					cnt = cnt + 1;
				}
				if (cnt == 0x10)
					break;
			}
			if (0x10 < i) {
				i = 0x11;
			}
			para->dram_para1 = i << 4 | uVar1 | uVar5 << 0xc | bg_num << 0xe;
			para->dram_tpr10 = uVar4;
			uVar4 = 1;
		}
	}
	return uVar4;
}

uint32_t auto_scan_dram_config(dram_para_t *para) {
	uint32_t uVar1;
	uint32_t ret_val;

	if (((para->dram_tpr13 & 0x4000) == 0) && (uVar1 = auto_scan_dram_rank_width(para), uVar1 == 0)) {
		return 0;
	}
	uVar1 = auto_scan_dram_size(para);
	if (uVar1 == 0) {
		uVar1 = 0;
	} else {
		if ((para->dram_tpr13 & 0x8000) == 0) {
			para->dram_tpr13 = para->dram_tpr13 | 3;
			para->dram_tpr13 = para->dram_tpr13 | 0x6000;
		}
		uVar1 = 1;
	}
	return uVar1;
}

uint32_t dramc_simple_wr_test(uint32_t dram_size, uint32_t test_length) {
	int iVar1;
	int iVar2;
	uint32_t val;
	uint32_t half_size;
	uint32_t reg_val;
	uint32_t i;

	iVar1 = (dram_size >> 1) * 0x100000;
	for (i = 0; i < test_length; i = i + 1) {
		*(uint32_t *) ((i + 0x10000000) * 4) = i + 0x1234567;
		*(uint32_t *) (iVar1 + i * 4 + 0x40000000) = i + 0xfedcba98;
	}
	i = 0;
	while (true) {
		if (test_length <= i) {
			printf("DRAM simple test OK.\n");
			return 0;
		}
		iVar2 = *(int *) (iVar1 + i * 4 + 0x40000000);
		if (iVar2 != i + 0xfedcba98)
			break;
		iVar2 = *(int *) ((i + 0x10000000) * 4);
		if (iVar2 != i + 0x1234567) {
			printf("DRAM simple test FAIL-----%x != %x at address %x\n", iVar2, i + 0x1234567, (i + 0x10000000) * 4);
			return 1;
		}
		i = i + 1;
	}
	printf("DRAM simple test FAIL-----%x != %x at address %x\n", iVar2, i + 0xfedcba98, iVar1 + i * 4 + 0x40000000);
	return 1;
}

uint32_t auto_cal_timing(uint32_t time_ns, uint32_t clk) {
	uint32_t value;

	return (uint32_t) ((time_ns * clk) % 1000 != 0) + (time_ns * clk) / 1000;
}

void mctl_com_set_controller_after_phy(dram_para_t *para) {
	uint32_t reg_val;

	REG32(0x04820320) = 0;
	mctl_com_set_controller_refresh(0, para);
	REG32(0x04820320) = 1;
	do {
	} while ((REG32(0x04820324) & 1) != 1);
	return;
}

void mctl_com_set_controller_before_phy(dram_para_t *para) {
	uint32_t reg_val;

	mctl_com_set_controller_refresh(1, para);
	REG32(0x048201b0) = REG32(0x048201b0) & 0xfffffffe;
	REG32(0x04820030) = 0x20;
	REG32(0x04810020) = REG32(0x04810020) | 0x100;
	return;
}

void mctl_com_set_controller_dbi(dram_para_t *para) {
	uint32_t reg_val;

	if ((para->dram_tpr13 & 0x20000000) != 0) {
		REG32(0x048201c0) = REG32(0x048201c0) | 4;
	}
	return;
}

void mctl_com_set_controller_update(dram_para_t *para) {
	uint32_t reg_val;

	REG32(0x048201a0) = REG32(0x048201a0) | 0xc0000000;
	REG32(0x04820180) = REG32(0x04820180) | 0xc0000000;
	REG32(0x04822180) = REG32(0x04822180) | 0xc0000000;
	REG32(0x04823180) = REG32(0x04823180) | 0xc0000000;
	REG32(0x04824180) = REG32(0x04824180) | 0xc0000000;
	return;
}

/* WARNING: Removing unreachable block (ram,0x00013e58) */
/* WARNING: Could not reconcile some variable overlaps */
void mctl_com_set_channel_timing(dram_para_t *para) {
	uint32_t uVar1;
	uint32_t uVar2;
	uint32_t uVar3;
	uint32_t uVar4;
	uint32_t type;
	uint32_t ctrl_freq;
	uint32_t i;
	uint8_t tccd_mw;
	uint8_t todton_min;
	uint8_t odtloff;
	uint8_t odtlon;
	uint8_t tckmpe;
	uint8_t tmpx_s;
	uint8_t tmpx_h;
	uint8_t txmpdll;
	uint8_t tgear_thd;
	uint8_t tgear_tsu;
	uint8_t tcmd_gear;
	uint8_t tsync_gear;
	uint8_t tdqsck_max;
	uint8_t twr;
	uint32_t freq_reg_offet;
	uint8_t txs_abort;
	uint8_t txs_fast;
	uint8_t txsdll;
	uint8_t txs;
	uint8_t txsr;
	uint8_t tmrd_pda;
	uint8_t twr2rd_s;
	uint8_t twtr_s;
	uint8_t trrd_s;
	uint8_t tccd_s;
	uint8_t tcksre;
	uint8_t tcksrx;
	uint8_t tckesr;
	uint8_t trd2wr;
	uint8_t twr2rd;
	uint8_t trasmax;
	uint8_t twtp;
	uint16_t _trfc;
	uint16_t trefi;
	uint8_t txp;
	uint8_t trtp;
	uint8_t twtr;
	uint8_t trp;
	uint8_t tras;
	uint8_t tfaw;
	uint8_t trc;
	uint8_t trcd;
	uint8_t trrd;
	uint8_t tcke;
	uint8_t tccd;
	uint8_t tmod;
	uint8_t tmrd;
	uint8_t tmrw;
	uint8_t tcwl;
	uint8_t tcl;
	uint8_t wr_latency;
	uint8_t t_rdata_en;
	uint32_t reg_val;

	t_rdata_en = '\x01';
	wr_latency = '\x01';
	tcl = '\x03';
	tcwl = '\x03';
	tmrw = '\0';
	tmrd = '\x02';
	tmod = '\x06';
	tccd = '\x02';
	tcke = '\x02';
	trrd = '\x03';
	trcd = '\x06';
	trc = '\x14';
	tfaw = '\x10';
	tras = '\x0e';
	trp = '\x06';
	twtr = '\x03';
	trtp = '\x03';
	txp = '\n';
	_trfc = 0x620080;
	twtp = '\f';
	trasmax = '\x1b';
	twr2rd = '\b';
	trd2wr = '\x04';
	tckesr = '\x03';
	tcksrx = '\x04';
	tcksre = '\x04';
	trrd_s = '\x02';
	twtr_s = '\x01';
	twr2rd_s = '\b';
	tmrd_pda = '\b';
	txsr = '\x04';
	txs = '\x04';
	txs_fast = '\x04';
	txs_abort = '\x04';
	uVar3 = para->dram_clk / 2;
	uVar4 = para->dram_type;
	if (uVar4 == 3) {
		tccd = '\x02';
		uVar1 = auto_cal_timing(0x32, uVar3);
		tfaw = (uint8_t) uVar1;
		uVar1 = auto_cal_timing(10, uVar3);
		trrd = (uint8_t) uVar1;
		if (trrd < 2) {
			trrd = '\x02';
		}
		uVar1 = auto_cal_timing(0xf, uVar3);
		trcd = (uint8_t) uVar1;
		uVar1 = auto_cal_timing(0x35, uVar3);
		trc = (uint8_t) uVar1;
		uVar1 = auto_cal_timing(8, uVar3);
		txp = (uint8_t) uVar1;
		if (txp < 2) {
			txp = '\x02';
		}
		uVar1 = auto_cal_timing(8, uVar3);
		twtr = (uint8_t) uVar1;
		if (twtr < 2) {
			twtr = '\x02';
		}
		uVar1 = auto_cal_timing(8, uVar3);
		trtp = (uint8_t) uVar1;
		if (trtp < 2) {
			trtp = '\x02';
		}
		auto_cal_timing(0xf, uVar3);
		uVar1 = auto_cal_timing(0xf, uVar3);
		trp = (uint8_t) uVar1;
		uVar1 = auto_cal_timing(0x26, uVar3);
		tras = (uint8_t) uVar1;
		uVar1 = auto_cal_timing(0x1e78, uVar3);
		uVar2 = auto_cal_timing(0x15e, uVar3);
		_trfc = uVar2 & 0xffff | (uVar1 >> 5) << 0x10;
		uVar1 = auto_cal_timing(0x168, uVar3);
		txs = (uint8_t) (uVar1 >> 5);
	} else if (uVar4 == 4) {
		tccd = '\x03';
		uVar1 = auto_cal_timing(0x23, uVar3);
		tfaw = (uint8_t) uVar1;
		uVar1 = auto_cal_timing(8, uVar3);
		trrd = (uint8_t) uVar1;
		if (trrd < 2) {
			trrd = '\x02';
		}
		uVar1 = auto_cal_timing(6, uVar3);
		trrd_s = (uint8_t) uVar1;
		if (trrd_s < 2) {
			trrd_s = '\x02';
		}
		uVar1 = auto_cal_timing(10, uVar3);
		tmrd_pda = (uint8_t) uVar1;
		if (tmrd_pda < 8) {
			tmrd_pda = '\b';
		}
		uVar1 = auto_cal_timing(0xf, uVar3);
		trcd = (uint8_t) uVar1;
		uVar1 = auto_cal_timing(0x31, uVar3);
		trc = (uint8_t) uVar1;
		uVar1 = auto_cal_timing(6, uVar3);
		txp = (uint8_t) uVar1;
		if (txp < 2) {
			txp = '\x02';
		}
		uVar1 = auto_cal_timing(8, uVar3);
		twtr = (uint8_t) uVar1;
		if (twtr < 2) {
			twtr = '\x02';
		}
		uVar1 = auto_cal_timing(3, uVar3);
		twtr_s = (uint8_t) uVar1;
		if (twtr_s == '\0') {
			twtr_s = '\x01';
		}
		uVar1 = auto_cal_timing(0xf, uVar3);
		trp = (uint8_t) uVar1;
		uVar1 = auto_cal_timing(0x22, uVar3);
		tras = (uint8_t) uVar1;
		uVar1 = auto_cal_timing(0x1e78, uVar3);
		uVar2 = auto_cal_timing(0x15e, uVar3);
		_trfc = uVar2 & 0xffff | (uVar1 >> 5) << 0x10;
		uVar1 = auto_cal_timing(0x168, uVar3);
		txs = (uint8_t) (uVar1 >> 5);
	} else if (uVar4 == 7) {
		tccd = '\x02';
		uVar1 = auto_cal_timing(0x32, uVar3);
		tfaw = (uint8_t) uVar1;
		if (tfaw < 4) {
			tfaw = '\x04';
		}
		uVar1 = auto_cal_timing(10, uVar3);
		trrd = (uint8_t) uVar1;
		if (trrd == '\0') {
			trrd = '\x01';
		}
		uVar1 = auto_cal_timing(0x18, uVar3);
		trcd = (uint8_t) uVar1;
		if (trcd < 2) {
			trcd = '\x02';
		}
		uVar1 = auto_cal_timing(0x46, uVar3);
		trc = (uint8_t) uVar1;
		uVar1 = auto_cal_timing(8, uVar3);
		txp = (uint8_t) uVar1;
		if (txp < 2) {
			txp = '\x02';
		}
		uVar1 = auto_cal_timing(8, uVar3);
		twtr = (uint8_t) uVar1;
		if (twtr < 2) {
			twtr = '\x02';
		}
		uVar1 = auto_cal_timing(8, uVar3);
		trtp = (uint8_t) uVar1;
		if (trtp < 2) {
			trtp = '\x02';
		}
		auto_cal_timing(0xf, uVar3);
		uVar1 = auto_cal_timing(0x1b, uVar3);
		trp = (uint8_t) uVar1;
		uVar1 = auto_cal_timing(0x2a, uVar3);
		tras = (uint8_t) uVar1;
		uVar1 = auto_cal_timing(0xf3c, uVar3);
		uVar2 = auto_cal_timing(0xd2, uVar3);
		_trfc = uVar2 & 0xffff | (uVar1 >> 5) << 0x10;
		uVar1 = auto_cal_timing(0xdc, uVar3);
		txsr = (uint8_t) uVar1;
	} else if (uVar4 == 8) {
		tccd = '\x04';
		uVar1 = auto_cal_timing(0x28, uVar3);
		tfaw = (uint8_t) uVar1;
		uVar1 = auto_cal_timing(10, uVar3);
		trrd = (uint8_t) uVar1;
		if (trrd < 2) {
			trrd = '\x02';
		}
		uVar1 = auto_cal_timing(0x12, uVar3);
		trcd = (uint8_t) uVar1;
		if (trcd < 2) {
			trcd = '\x02';
		}
		uVar1 = auto_cal_timing(0x41, uVar3);
		trc = (uint8_t) uVar1;
		uVar1 = auto_cal_timing(8, uVar3);
		txp = (uint8_t) uVar1;
		if (txp < 2) {
			txp = '\x02';
		}
		uVar1 = auto_cal_timing(10, uVar3);
		twtr = (uint8_t) uVar1;
		if (twtr < 4) {
			twtr = '\x04';
		}
		uVar1 = auto_cal_timing(8, uVar3);
		trtp = (uint8_t) uVar1;
		if (trtp < 4) {
			trtp = '\x04';
		}
		auto_cal_timing(0x12, uVar3);
		uVar1 = auto_cal_timing(0x15, uVar3);
		trp = (uint8_t) uVar1;
		uVar1 = auto_cal_timing(0x2a, uVar3);
		tras = (uint8_t) uVar1;
		uVar1 = auto_cal_timing(0xf40, uVar3);
		uVar2 = auto_cal_timing(0xb4, uVar3);
		_trfc = uVar2 & 0xffff | (uVar1 >> 5) << 0x10;
		uVar1 = auto_cal_timing(0xbe, uVar3);
		txsr = (uint8_t) uVar1;
	}
	switch (uVar4) {
		case 3:
			tmrw = '\0';
			tmrd = '\x04';
			tmod = '\f';
			uVar1 = auto_cal_timing(8, uVar3);
			tcke = (uint8_t) uVar1;
			if (tcke < 2) {
				tcke = '\x02';
			}
			uVar1 = auto_cal_timing(10, uVar3);
			tcksrx = (uint8_t) uVar1;
			if (tcksrx < 3) {
				tcke = '\x06';
			}
			uVar1 = auto_cal_timing(10, uVar3);
			tcksre = (uint8_t) uVar1;
			if (tcksre < 3) {
				tcke = '\x06';
			}
			tckesr = tcke + '\x01';
			trasmax = (uint8_t) (uVar3 / 0xf);
			tcl = '\a';
			tcwl = '\x05';
			t_rdata_en = '\n';
			wr_latency = '\x06';
			para->dram_mr0 = 0x1f14;
			para->dram_mr2 = 0x20;
			para->dram_mr3 = 0;
			if ((uint32_t) trp + (uint32_t) trtp < 9) {
				trtp = '\t' - trp;
			}
			twtp = '\x0e';
			twr2rd = twtr + '\a';
			trd2wr = '\x05';
			break;
		case 4:
			tmrw = '\0';
			tmrd = '\x04';
			uVar1 = auto_cal_timing(0xf, uVar3);
			tmod = (uint8_t) uVar1;
			if (tmod < 0xc) {
				tmod = '\f';
			}
			uVar1 = auto_cal_timing(5, uVar3);
			tcke = (uint8_t) uVar1;
			if (tcke < 2) {
				tcke = '\x02';
			}
			uVar1 = auto_cal_timing(10, uVar3);
			tcksrx = (uint8_t) uVar1;
			if (tcksrx < 3) {
				tcksrx = '\x03';
			}
			uVar1 = auto_cal_timing(10, uVar3);
			tcksre = (uint8_t) uVar1;
			if (tcksre < 3) {
				tcksre = '\x03';
			}
			tckesr = tcke + '\x01';
			uVar1 = auto_cal_timing(0xaa, uVar3);
			txs_fast = (uint8_t) (uVar1 >> 5);
			uVar1 = auto_cal_timing(0xaa, uVar3);
			txs_abort = (uint8_t) (uVar1 >> 5);
			uVar3 = auto_cal_timing(0x11238, uVar3);
			trasmax = (uint8_t) (uVar3 >> 10);
			tcl = '\a';
			tcwl = '\x05';
			t_rdata_en = '\n';
			wr_latency = '\x06';
			para->dram_mr0 = 0x520;
			para->dram_mr2 = 8;
			trtp = '\x04';
			if (trp + 4 < 9) {
				trtp = '\t' - trp;
			}
			twtp = '\x0e';
			twr2rd = twtr + '\a';
			twr2rd_s = twtr_s + '\a';
			trd2wr = '\x05';
			break;
		case 7:
			tmrw = '\x05';
			tmrd = '\x05';
			tmod = '\f';
			tcke = '\x03';
			tcksrx = '\x05';
			tcksre = '\x05';
			tckesr = '\x05';
			trasmax = '\x18';
			tcl = '\a';
			tcwl = '\x04';
			t_rdata_en = '\f';
			wr_latency = '\x06';
			para->dram_mr1 = 0x83;
			para->dram_mr2 = 0x1c;
			para->dram_mr0 = 0;
			twtp = '\x10';
			trd2wr = '\r';
			twr2rd = twtr + '\t';
			break;
		case 8:
			uVar1 = auto_cal_timing(0xe, uVar3);
			tmrw = (uint8_t) uVar1;
			if (tmrw < 5) {
				tmrw = '\x05';
			}
			uVar1 = auto_cal_timing(0xe, uVar3);
			tmrd = (uint8_t) uVar1;
			if (tmrd < 5) {
				tmrd = '\x05';
			}
			tmod = '\f';
			uVar1 = auto_cal_timing(0xf, uVar3);
			tcke = (uint8_t) uVar1;
			if (tcke < 2) {
				tcke = '\x02';
			}
			uVar1 = auto_cal_timing(2, uVar3);
			tcksrx = (uint8_t) uVar1;
			if (tcksrx < 2) {
				tcksrx = '\x02';
			}
			uVar1 = auto_cal_timing(5, uVar3);
			tcksre = (uint8_t) uVar1;
			if (tcksre < 2) {
				tcksre = '\x02';
			}
			uVar1 = auto_cal_timing(0xf, uVar3);
			tckesr = (uint8_t) uVar1;
			if (tckesr < 2) {
				tckesr = '\x02';
			}
			trasmax = (uint8_t) ((int) ((_trfc >> 0x10) * 9) >> 5);
			uVar1 = auto_cal_timing(4, uVar3);
			uVar3 = auto_cal_timing(1, uVar3);
			tcl = '\n';
			tcwl = '\x05';
			t_rdata_en = '\x11';
			wr_latency = '\x05';
			para->dram_mr1 = 0x34;
			para->dram_mr2 = 0x1b;
			trtp = '\x04';
			twtp = '\x18';
			trd2wr = ((char) uVar1 - (char) uVar3) + '\x11';
			twr2rd = twtr + '\x0e';
	}
	REG32(0x04820100) = (uint32_t) tras | (uint32_t) twtp << 0x18 | (uint32_t) tfaw << 0x10 | (uint32_t) trasmax << 8;
	REG32(0x04820104) = (uint32_t) trc | (uint32_t) txp << 0x10 | (uint32_t) trtp << 8;
	REG32(0x04820108) = (uint32_t) twr2rd | (uint32_t) tcwl << 0x18 | (uint32_t) tcl << 0x10 | (uint32_t) trd2wr << 8;
	REG32(0x0482010c) = (uint32_t) tmod | (uint32_t) tmrw << 0x14 | (uint32_t) tmrd << 0xc;
	REG32(0x04820110) = (uint32_t) trp | (uint32_t) trcd << 0x18 | (uint32_t) tccd << 0x10 | (uint32_t) trrd << 8;
	REG32(0x04820114) = (uint32_t) tcke | (uint32_t) tcksrx << 0x18 | (uint32_t) tcksre << 0x10 | (uint32_t) tckesr << 8;
	REG32(0x04820118) = txp + 2 | 0x2020000;
	REG32(0x04820120) = (uint32_t) txs | (uint32_t) txs_fast << 0x18 | (uint32_t) txs_abort << 0x10 | 0x1000;
	REG32(0x04820124) = (uint32_t) twr2rd_s | (uint32_t) trrd_s << 8 | 0x20000;
	REG32(0x04820128) = 0xe0c05;
	REG32(0x0482012c) = 0x440c021c;
	REG32(0x04820130) = (uint32_t) tmrd_pda;
	REG32(0x04820134) = 0xa100002;
	REG32(0x04820138) = (uint32_t) txsr;
	reg_val = REG32(0x048200d0) & 0x3fffffff;
	if (uVar4 == 7) {
		reg_val = REG32(0x048200d0) & 0x3c00ffff | 0x4f0000;
	}
	if (uVar4 == 8) {
		reg_val = reg_val & 0xfffff000 | 0x3f0;
	} else {
		reg_val = reg_val & 0xfffff000 | 200;
	}
	REG32(0x048200d0) = reg_val;
	REG32(0x048200d4) = 0x420000;
	REG32(0x048200d8) = 5;
	REG32(0x048201b0) = 0;
	if (((uVar4 == 6) || (uVar4 == 7)) || (uVar4 == 8)) {
		REG32(0x048200dc) = para->dram_mr1 << 0x10 | para->dram_mr2;
		REG32(0x048200e0) = para->dram_mr3 << 0x10;
	} else {
		REG32(0x048200dc) = para->dram_mr0 << 0x10 | para->dram_mr1;
		REG32(0x048200e0) = para->dram_mr2 << 0x10 | para->dram_mr3;
	}
	if (uVar4 == 4) {
		REG32(0x048200e8) = para->dram_mr4 << 0x10 | para->dram_mr5;
		REG32(0x048200ec) = para->dram_mr6;
	}
	if (uVar4 == 8) {
		REG32(0x048200e8) = para->dram_mr11 << 0x10 | para->dram_mr12;
		REG32(0x048200ec) = para->dram_mr22 << 0x10 | para->dram_mr14;
	}
	REG32(0x048200f4) = REG32(0x048200f4) & 0xfffff00f | 0x660;
	if ((para->dram_tpr13 & 0x20) == 0) {
		uVar3 = wr_latency - 1 | (t_rdata_en - 1) * 0x10000 | 0x2000000;
	} else {
		uVar3 = (uint32_t) wr_latency | (uint32_t) t_rdata_en << 0x10 | 0x2000000;
	}
	reg_val = uVar3 | 0x808000;
	REG32(0x04820190) = reg_val;
	REG32(0x04820194) = 0x100202;
	REG32(0x04820064) = _trfc;
	return;
}

void mctl_com_set_controller_address_map(dram_para_t *para) {
	uint32_t uVar1;
	uint32_t uVar2;
	uint32_t uVar3;
	uint32_t uVar4;
	uint32_t bg_num;
	uint32_t bank_num;
	uint32_t row_num;
	uint32_t col_num;

	uVar1 = para->dram_para1 & 0xf;
	uVar2 = para->dram_para1 >> 4 & 0xff;
	uVar3 = para->dram_para1 >> 0xc & 3;
	uVar4 = para->dram_para1 >> 0xe & 3;
	if ((para->dram_para2 & 0xf) != 0) {
		uVar1 = uVar1 - 1;
	}
	REG32(0x04820208) = uVar4 << 0x18 | uVar4 << 8 | uVar4 << 0x10;
	switch (uVar1) {
		case 8:
			REG32(0x0482020c) = uVar4 | uVar4 << 8 | 0x1f1f0000;
			REG32(0x04820210) = 0x1f1f;
			break;
		case 9:
			REG32(0x0482020c) = uVar4 << 8 | uVar4 | uVar4 << 0x10 | 0x1f000000;
			REG32(0x04820210) = 0x1f1f;
			break;
		case 10:
			REG32(0x0482020c) = uVar4 << 0x18 | uVar4 << 8 | uVar4 | uVar4 << 0x10;
			REG32(0x04820210) = 0x1f1f;
			break;
		case 0xb:
			REG32(0x0482020c) = uVar4 << 0x18 | uVar4 << 8 | uVar4 | uVar4 << 0x10;
			REG32(0x04820210) = uVar4 | 0x1f00;
			break;
		default:
			REG32(0x0482020c) = uVar4 << 0x18 | uVar4 << 8 | uVar4 | uVar4 << 0x10;
			REG32(0x04820210) = uVar4 | uVar4 << 8;
	}
	if (uVar4 == 2) {
		REG32(0x04820220) = 0x101;
	} else if (uVar4 == 1) {
		REG32(0x04820220) = 0x3f01;
	} else {
		REG32(0x04820220) = 0x3f3f;
	}
	if (uVar3 == 3) {
		REG32(0x04820204) = (uVar1 + uVar4 + -2) * 0x10000 | (uVar1 + uVar4) - 2 | (uVar1 + uVar4 + -2) * 0x100;
	} else {
		REG32(0x04820204) = (uVar1 + uVar4) - 2 | (uVar1 + uVar4 + -2) * 0x100 | 0x3f0000;
	}
	REG32(0x04820214) = (uVar3 + uVar4 + uVar1 + -6) * 0x1000000 | (uVar3 + uVar4 + uVar1) - 6 | (uVar3 + uVar4 + uVar1 + -6) * 0x100 | (uVar3 + uVar4 + uVar1 + -6) * 0x10000;
	switch (uVar2) {
		case 0xe:
			REG32(0x04820218) = (uVar3 + uVar4 + uVar1 + -6) * 0x100 | (uVar3 + uVar4 + uVar1) - 6 | 0xf0f0000;
			REG32(0x0482021c) = 0xf0f;
			break;
		case 0xf:
			REG32(0x04820218) = (uVar3 + uVar4 + uVar1) - 6 | (uVar3 + uVar4 + uVar1 + -6) * 0x100 | (uVar3 + uVar4 + uVar1 + -6) * 0x10000 | 0xf000000;
			REG32(0x0482021c) = 0xf0f;
			break;
		case 0x10:
			REG32(0x04820218) =
					(uVar3 + uVar4 + uVar1 + -6) * 0x1000000 | (uVar3 + uVar4 + uVar1) - 6 | (uVar3 + uVar4 + uVar1 + -6) * 0x100 | (uVar3 + uVar4 + uVar1 + -6) * 0x10000;
			REG32(0x0482021c) = 0xf0f;
			break;
		case 0x11:
			REG32(0x04820218) =
					(uVar3 + uVar4 + uVar1 + -6) * 0x1000000 | (uVar3 + uVar4 + uVar1) - 6 | (uVar3 + uVar4 + uVar1 + -6) * 0x100 | (uVar3 + uVar4 + uVar1 + -6) * 0x10000;
			REG32(0x0482021c) = (uVar3 + uVar4 + uVar1) - 6 | 0xf00;
			break;
		default:
			REG32(0x04820218) =
					(uVar3 + uVar4 + uVar1 + -6) * 0x1000000 | (uVar3 + uVar4 + uVar1) - 6 | (uVar3 + uVar4 + uVar1 + -6) * 0x100 | (uVar3 + uVar4 + uVar1 + -6) * 0x10000;
			REG32(0x0482021c) = (uVar3 + uVar4 + uVar1 + -6) * 0x100 | (uVar3 + uVar4 + uVar1) - 6;
	}
	if ((para->dram_para2 & 0x1000) == 0) {
		REG32(0x04820200) = 0x1f;
	} else {
		REG32(0x04820200) = uVar4 + uVar1 + uVar3 + uVar2 + -6;
	}
	return;
}

void mctl_com_set_controller_odt(dram_para_t *para) {
	uint32_t uVar1;
	uint32_t t_odt;
	uint32_t wl;
	uint32_t reg_val;

	if ((para->dram_para2 & 0x1000) == 0) {
		reg_val = 0x201;
	} else {
		reg_val = 0x303;
	}
	REG32(0x04820244) = reg_val;
	reg_val = 0x4000400;
	uVar1 = para->dram_type & 7;
	if (uVar1 == 7) {
		if (para->dram_clk < 400) {
			wl = 3;
		} else {
			wl = 4;
		}
		reg_val = (wl - (int) (para->dram_clk * 7) / 2000) * 0x10000 | ((int) (para->dram_clk * 7) / 2000 + 7) * 0x1000000 | 0x400U;
	} else if (uVar1 < 8) {
		if (uVar1 == 3) {
			reg_val = 0x6000400;
		} else if (uVar1 == 4) {
			reg_val = (para->dram_mr4 >> 6 & 7) << 0x10 | ((para->dram_mr4 >> 0xc & 1) + 6) * 0x1000000 | 0x400;
		}
	}
	REG32(0x04820240) = reg_val;
	REG32(0x04822240) = reg_val;
	REG32(0x04823240) = reg_val;
	REG32(0x04824240) = reg_val;
	return;
}

void mctl_com_set_controller_2T_mode(dram_para_t *para) {
	uint32_t reg_val;

	if ((REG32(0x04820000) & 0x800) == 0) {
		if ((para->dram_tpr13 & 0x20) == 0) {
			reg_val = REG32(0x04820000) | 0x400;
		} else {
			reg_val = REG32(0x04820000) & 0xfffffbff;
		}
	} else {
		reg_val = REG32(0x04820000) & 0xfffffbff;
	}
	REG32(0x04820000) = reg_val;
	return;
}

void mctl_com_set_controller_geardown_mode(dram_para_t *para) {
	uint32_t reg_val;

	REG32(0x04820000) = para->dram_tpr13 >> 0x1e & 1 | REG32(0x04820000);
	return;
}

void mctl_com_set_controller_config(dram_para_t *para) {
	uint32_t reg_val;

	switch (para->dram_type & 0xf) {
		case 3:
			reg_val = 0x40001;
			break;
		case 4:
			reg_val = 0x40010;
			break;
		default:
			reg_val = 0x40001;
			break;
		case 7:
			reg_val = 0x40008;
			break;
		case 8:
			reg_val = 0x80020;
	}
	REG32(0x04820000) = ((para->dram_para2 >> 0xc & 3) * 2 + 1) * 0x1000000 | (para->dram_para2 & 1) << 0xc | reg_val | 0xc0000000;
	return;
}

void mctl_com_init(dram_para_t *para) {
	mctl_com_set_controller_config(para);
	if (para->dram_type == 4) {
		mctl_com_set_controller_geardown_mode(para);
	}
	if ((para->dram_type == 3) || (para->dram_type == 4)) {
		mctl_com_set_controller_2T_mode(para);
	}
	mctl_com_set_controller_odt(para);
	mctl_com_set_controller_address_map(para);
	mctl_com_set_channel_timing(para);
	REG32(0x04820030) = 0;
	mctl_com_set_controller_update(para);
	if ((para->dram_type == 4) || (para->dram_type == 8)) {
		mctl_com_set_controller_dbi(para);
	}
	mctl_com_set_controller_before_phy(para);
	return;
}

void mctl_com_set_bus_config(dram_para_t *para) {
	uint32_t reg_val;

	if (para->dram_type == 8) {
		REG32(0x03102ea8) = REG32(0x03102ea8) | 1;
	}
	REG32(0x04820250) = REG32(0x04820250) & 0xffff00ff | 0x3000;
	return;
}

void mctl_sys_init(dram_para_t *para) {
	uint32_t reg_val;

	REG32(0x03001540) = REG32(0x03001540) & 0x3fffffff;
	REG32(0x0300180c) = REG32(0x0300180c) & 0xfffefffe;
	REG32(0x03001010) = REG32(0x03001010) & 0x7fffffff;
	REG32(0x03001800) = REG32(0x03001800) & 0xbfffffff;
	udelay(5);
	_ccm_set_pll_ddr_clk(para->dram_clk << 1, para);
	REG32(0x0300180c) = REG32(0x0300180c) | 0x10001;
	REG32(0x03001540) = REG32(0x03001540) | 0xc0000000;
	REG32(0x04810008) = REG32(0x04810008) & 0xfdffffff;
	REG32(0x03001800) = REG32(0x03001800) & 0xfcffffe0 | 0x48000003;
	udelay(5);
	REG32(0x07010250) = REG32(0x07010250) | 0x10;
	return;
}

int init_DRAM(dram_para_t *para) {
	uint32_t uVar1;
	uint32_t reg_val;
	uint32_t ret_val;
	uint32_t dram_size;

	printf("DRAM BOOT DRIVE INFO: %s\n", "V0.15");
	REG32(0x03000160) = REG32(0x03000160) | 0x100;
	REG32(0x03000168) = REG32(0x03000168) & 0xffffffc0;
	if (((para->dram_tpr13 & 1) == 0) && (uVar1 = auto_scan_dram_config(para), uVar1 == 0)) {
		dram_size = 0;
	} else {
		printf("DRAM CLK =%d MHZ\n", para->dram_clk);
		printf("DRAM Type =%d (3:DDR3,4:DDR4,7:LPDDR3,8:LPDDR4)\n", para->dram_type);
		uVar1 = mctl_core_init(para);
		if (uVar1 == 0) {
			printf("DRAM initial error : 1 !\n");
			dram_size = 0;
		} else {
			if ((int) para->dram_para2 < 0) {
				dram_size = para->dram_para2 >> 0x10 & 0x7fff;
			} else {
				dram_size = DRAMC_get_dram_size(para);
				para->dram_para2 = para->dram_para2 & 0xffff | dram_size << 0x10;
			}
			printf("DRAM SIZE =%d MBytes, para1 = %x, para2 = %x, dram_tpr13 = %x\n", dram_size, para->dram_para1, para->dram_para2, para->dram_tpr13);
			if ((para->dram_tpr13 & 0x1000000) != 0) {
				REG32(0x04820030) = REG32(0x04820030) | 9;
			}
			uVar1 = dramc_simple_wr_test(dram_size, 0x1000);
			if (uVar1 != 0) {
				dram_size = 0;
			}
		}
	}
	return dram_size;
}

uint32_t sunxi_dram_init(void *para) { return init_DRAM((dram_para_t *) para); }