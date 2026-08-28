/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_SPIF_SPI_MEM_H__
#define __DRIVERS_SPIF_SPI_MEM_H__

#include <stdint.h>
#include <types.h>

#define SPI_MEM_OP_CMD(__opcode, __buswidth) { .buswidth = (__buswidth), .opcode = (__opcode), .nbytes = 1 }

#define SPI_MEM_OP_NO_CMD \
	{                 \
	}

#define SPI_MEM_OP_ADDR(__nbytes, __val, __buswidth) { .nbytes = (__nbytes), .val = (__val), .buswidth = (__buswidth) }

#define SPI_MEM_OP_NO_ADDR \
	{                  \
	}

#define SPI_MEM_OP_MODE(__val, __buswidth) { .val = (__val), .buswidth = (__buswidth) }

#define SPI_MEM_OP_NO_MODE \
	{                  \
	}

#define SPI_MEM_OP_DUMMY(__nbytes, __buswidth) { .nbytes = (__nbytes), .buswidth = (__buswidth) }

#define SPI_MEM_OP_NO_DUMMY \
	{                   \
	}

#define SPI_MEM_OP_DATA_IN(__nbytes, __buf, __buswidth) \
	{ .dir = SPI_MEM_DATA_IN, .nbytes = (__nbytes), .buf.in = (__buf), .buswidth = (__buswidth) }

#define SPI_MEM_OP_DATA_OUT(__nbytes, __buf, __buswidth) \
	{ .dir = SPI_MEM_DATA_OUT, .nbytes = (__nbytes), .buf.out = (__buf), .buswidth = (__buswidth) }

#define SPI_MEM_OP_NO_DATA \
	{                  \
	}

enum spi_mem_data_dir {
	SPI_MEM_NO_DATA,
	SPI_MEM_DATA_IN,
	SPI_MEM_DATA_OUT,
};

struct spi_mem_op {
	struct {
		u8 nbytes;
		u8 buswidth;
		u8 dtr : 1;
		u16 opcode;
	} cmd;

	struct {
		u8 nbytes;
		u8 buswidth;
		u8 dtr : 1;
		u64 val;
	} addr;

	struct {
		u8 buswidth;
		const void *val;
	} mode;

	struct {
		u8 nbytes;
		u8 buswidth;
		u8 dtr : 1;
	} dummy;

	struct {
		u8 buswidth;
		u8 dtr : 1;
		enum spi_mem_data_dir dir;
		u32 nbytes;
		union {
			void *in;
			const void *out;
		} buf;
	} data;
};

#define SPI_MEM_OP(__cmd, __addr, __mode, __dummy, __data) \
	{ .cmd = (__cmd), .addr = (__addr), .mode = (__mode), .dummy = (__dummy), .data = (__data) }

#endif /* __DRIVERS_SPIF_SPI_MEM_H__ */
