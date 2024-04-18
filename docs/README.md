# SyterKit Intro

![SyterKit LOGO_Thin](https://github.com/YuzukiHD/SyterKit/assets/12003087/e6135860-1a6a-4cb4-b0f6-71af8eca1509)

SyterKit is a bare-metal framework designed for Allwinner platform. SyterKit utilizes CMake as its build system and supports various applications and peripheral drivers. Additionally, SyterKit also has bootloader functionality, which enables it to replace U-Boot for fast booting (standard Linux 6.7 mainline boot time of 1.02s, significantly faster than traditional U-Boot's 3s boot time).

# Support list

| Board                                                        | Manufacturer | Platform | Spec                              | Details                                        | Config                  |
| ------------------------------------------------------------ | ------------ | -------- | --------------------------------- | ---------------------------------------------- | ----------------------- |
| [Yuzukilizard](https://github.com/YuzukiHD/Yuzukilizard)     | YuzukiHD     | V851s    | Cortex A7                         | [board/yuzukilizard](board/yuzukilizard)       | `yuzukilizard.cmake`    |
| [TinyVision](https://github.com/YuzukiHD/TinyVision)         | YuzukiHD     | V851se   | Cortex A7                         | [board/tinyvision](board/tinyvision)           | `tinyvision.cmake`      |
| 100ask-t113s3                                                | 100ask       | T113-S3  | Dual-Core Cortex A7               | [board/100ask-t113s3](board/100ask-t113s3)     | `100ask-t113s3.cmake`   |
| 100ask-t113i                                                 | 100ask       | T113-I   | Dual-Core Cortex A7 + C906 RISC-V | [board/100ask-t113i](board/100ask-t113i)       | `100ask-t113i.cmake`    |
| 100ask-d1-h                                                  | 100ask       | D1-H     | C906 RISC-V                       | [board/100ask-d1-h](board/100ask-d1-h)         | `100ask-d1-h.cmake`     |
| dongshanpi-aict                                              | 100ask       | V853     | Cortex A7                         | [board/dongshanpi-aict](board/dongshanpi-aict) | `dongshanpi-aict.cmake` |
| project-yosemite                                             | YuzukiHD     | V853     | Cortex A7                         | [board/project-yosemite](board/project-yosemite) | `project-yosemite.cmake` |
| 100ask ROS                                                   | 100ask       | R818     | Quad-Core Cortex A53              | [board/100ask-ros](board/100ask-ros)           | `100ask-ros.cmake`      |
| [longanpi-3h](https://wiki.sipeed.com/hardware/zh/longan/H618/lpi3h/1_intro.html) | sipeed       | H618     | Quad-Core Cortex A53              | [board/longanpi-3h](board/longanpi-3h)         | `longanpi-3h.cmake`     |
| longanpi-4b                                                  | sipeed       | T527     | Octa-Core Cortex A55              | [board/longanpi-4b](board/longanpi-4b)         | `longanpi-4b.cmake`     |
| [LT527X](https://www.myir.cn/shows/134/70.html)              | myir-tech    | T527     | Octa-Core Cortex A55 | [board/lt527x](board/lt527x) | `lt527x.cmake` |
|  Avaota A1                                                   | YuzukiHD     | T527     | Octa-Core Cortex A55 | [board/avaota-a1](board/avaota-a1) | `avaota-a1.cmake` |
| OrangePi 4A | OrangePi | A527 | Octa-Core Cortex A55 | (可以嫖个板子吗) | - |

# Getting Started

## Building SyterKit From Scratch

Building SyterKit is a straightforward process that only requires setting up the environment for compilation on a Linux operating system. The software packages required by SyterKit include:

- `gcc-arm-none-eabi`
- `CMake`

For commonly used Ubuntu systems, they can be installed using the following command:

```shell
sudo apt-get update
sudo apt-get install gcc-arm-none-eabi cmake build-essential -y
```

Then create a folder to store the compiled output files and navigate to it:

```shell
mkdir build
cd build
```

Finally, run the following commands to compile SyterKit:

```shell
cmake -DCMAKE_BOARD_FILE={Board_config_file.cmake} ..
make
```

For example, if you want to compile SyterKit for the TinyVision platform, you need the following command:

```bash
cmake -DCMAKE_BOARD_FILE=tinyvision.cmake ..
make
```

The compiled executable files will be located in `build/board/{board_name}/{app_name}`.

The SyterKit project will compile two versions: firmware ending with `.elf` is for USB booting and requires bootloading by PC-side software, while firmware ending with `.bin` is for flashing and can be written into storage devices such as TF cards and SPI NAND.

- For SD Card, You need to flash the `xxx_card.bin`
- For SPI NAND/SPI NOR, You need to flash the `xxx_spi.bin`

## Creating TF Card Boot Firmware

After build the firmware, you can flash it into the TF card. For the V851s platform, you can write it to either an 8K offset or a 128K offset. Generally, if the TF card uses MBR format, write it with an 8K offset. If it uses GPT format, write it with a 128K offset. Assuming `/dev/sdb` is the target TF card, you can use the following command to write it with an 8K offset:

```shell
sudo dd if=syter_boot_bin_card.bin of=/dev/sdb bs=1024 seek=8
```

If it is a GPT partition table, you need to write it with a 128K offset:

```shell
sudo dd if=syter_boot_bin_card.bin of=/dev/sdb bs=1024 seek=128
```

### Creating the Firmware for SPI NAND

For SPI NAND, we need to create the firmware for SPI NAND by writing SyterKit to the corresponding positions:

```shell
dd if=syter_boot_bin_spi.bin of=spi.img bs=2k
dd if=syter_boot_bin_spi.bin of=spi.img bs=2k seek=32
dd if=syter_boot_bin_spi.bin of=spi.img bs=2k seek=64
```

You can also include the Linux kernel and device tree in the firmware:

```shell
dd if=sunxi.dtb of=spi.img bs=2k seek=128     # DTB on page 128
dd if=zImage of=spi.img bs=2k seek=256        # Kernel on page 256
```

Use the xfel tool to flash the created firmware into SPI NAND:

```shell
xfel spinand write 0x0 spi.img
```

### Creating the Firmware for SPI NOR

For SPI NOR, we need to create the firmware for SPI NOR by writing SyterKit to the corresponding positions:

```shell
dd if=syter_boot_bin_spi.bin of=spi.img bs=2k
dd if=syter_boot_bin_spi.bin of=spi.img bs=2k seek=32
dd if=syter_boot_bin_spi.bin of=spi.img bs=2k seek=64
```

You can also include the Linux kernel and device tree in the firmware:

```shell
dd if=sunxi.dtb of=spi.img bs=2k seek=128     # DTB on page 128
dd if=zImage of=spi.img bs=2k seek=256        # Kernel on page 256
```

Use the xfel tool to flash the created firmware into SPI NOR:

```shell
xfel spinor write 0x0 spi.img
```

# Development

## How `HELL` the device boot up

The device boot as noted over several places, BROM is the first step in booting and is baked into chip itself. Moving from the BROM, Allwinner boots something called boot0 from a storage device such as TF Card, SPI NAND, SPI NOR and so on. The magicvalue for the AllWinner bootloader in various places is 'eGON' and thus the bootloader shall be known as such. The chip will read the valided code from storage device to SRAM and execute the code in SRAM, the load lenth and start address in SRAM are defined in `boot_file_head`.

### Boot Head

#### boot_file_head

The define of `boot_file_head` used to detect as follow:

```c
typedef struct boot_file_head {
    uint32_t jump_instruction; /* one intruction jumping to real code */
    uint8_t magic[8];          /* ="eGON.BT0" */
    uint32_t check_sum;        /* generated by PC */
    uint32_t *length;          /* generated by LD */
    uint32_t pub_head_size;    /* the size of boot_file_head_t */
    uint8_t pub_head_vsn[4];   /* the version of boot_file_head_t */
    uint32_t *ret_addr;        /* the return value */
    uint32_t *run_addr;        /* run addr */
    uint32_t boot_cpu;         /* eGON version */
    uint8_t platform[8];       /* platform information */
} boot_file_head_t;
```

#### disassemble of boot_file_head

If you disassemble a compiled SyterKit binary, you will find that the head of the program is not the startup code but a startup header. This startup header is used by BROM to identify the size and length of the code that needs to be started and to verify it.

Here is a disassembly part of helloworld_bin, we focous on the boot_file_head part:

```asm
helloworld_bin:     file format elf32-littlearm
Disassembly of section .text:

00044000 <boot_head>:
   44000:	ea00000e 	b	44040 <_start> # jump_instruction, jump to the actrucl code 0x44040
   44004:	4e4f4765 	                   # magic 'e' 'G' 'O' 'N'
   44008:	3054422e 	                   # magic '.' 'B' 'T' '0'
   4400c:	12345678 	                   # check sum, need reset after padding, now use 0x12345678 to hold the place
   44010:	0000c0c8 	                   # length of the code
   44014:	00000030 	                   # the size of boot_file_head_t
   44018:	30303033 	                   # the version of boot_file_head_t
   4401c:	00044000 	                   # the return value, not used in SyterKit, set same as start address
   44020:	00044000 	                   # start address
   44024:	00000000 	                   # eGON version, set to 0x0
   44028:	2e330000 	                   # platform 0 0 '3' '.'
   4402c:	00302e30 	                   # platform '0' '.' '0' 0
	...

00044040 <_start>:
   44040:	ea000014 	b	44098 <reset>  # the actrucl start code
   44044:	e320f000 	nop	{0}
   44048:	e320f000 	nop	{0}
   4404c:	e320f000 	nop	{0}
   44050:	e320f000 	nop	{0}
   44054:	e320f000 	nop	{0}
   44058:	e320f000 	nop	{0}
   4405c:	e320f000 	nop	{0}
    ...
    ...
    ...
```

From the above, it is known that the first instruction at the program entry point is a jump instruction, which skips the boot_head to execute the actual startup code. BROM then performs verification of SyterKit by reading boot_head and retrieves the corresponding code to the corresponding address.

### eGON.BT0

In SyterKit supported platform, The BROM will seek the `eGON.BT0` magic from device storage. 

#### MMC Device

For the MMC Device, Such as TF Card, SD NAND, eMMC, UFS, the mapping as follows:

```
+--------+----------+------------------------------+
| 0K--8K |              8K----END                  |
+--------+----------+------------------------------+
|  MBR   | SyterKit |         Other Code           |
+--------+----------+------------------------------+
```

But for the GPT partition table, Due to 8K can't fit the GPT partition table and backup table, the chip support to check the header in 128K

```
+----------+-----------------------------------------+
| 0K--128K |             128K----END                 |
+----------+-----------------------------------------+
|   GPT    | SyterKit |         Other Code           |
+----------+----------+------------------------------+
```

The 8KB offset is dictated by the BROM, it will check for a valid eGON header at this location. if no valid eGON signature is found at 8KB, BROM can also check the header from sector 256 (128KB) of an SD card or eMMC. 

#### MTD Device

For the MTD Device, Such as SPI NAND, SPI NOR, the mapping as follows:

```
+--------------------------------------------------+
|                   0K------END                    |
+-------------------+------------------------------+
|     SyterKit      |         Other Code           |
+-------------------+------------------------------+
```

MTD Device is sample, check a valid eGON header and weeeeeee it run!

#### RAW Device

SyterKit does not Support NFTL Raw NAND.

#### About the jump_instruction for ARM

The jump_instruction field stores a jump instruction: (B  BACK_OF_boot_file_head_address). When this jump instruction is executed, the program will jump to the first instruction after the boot_file_head.

The B instruction encoding in ARM instructions is as follows:

```
+--------+---------+------------------------------+
| 31--28 | 27--24  |            23--0             |
+--------+---------+------------------------------+
|  cond  | 1 0 1 0 |        signed_immed_24       |
+--------+---------+------------------------------+
```

The "ARM Architecture Reference Manual" explains this instruction as follows:

```
Syntax:
B{<cond>}  <target_address>
  <cond>    Is the condition under which the instruction is executed. If the
            <cond> is omitted, the AL (always, its code is 0b1110) is used.
  <target_address>
            Specifies the address to branch to. The branch target address is
            calculated by:
            1.  Sign-extending the 24-bit signed (two's complement) immediate
                to 32 bits.
            2.  Shifting the result left two bits.
            3.  Adding to the contents of the PC, which contains the address
                of the branch instruction plus 8.
```

Based on this explanation, the highest 8 bits of the instruction encoding are: 0b11101010, and the lower 24 bits are generated dynamically based on the size of boot_file_head. Therefore, the assembly process of the instruction is as follows:

```c 
((((((sizeof(boot_file_head_t) + sizeof(int)) / sizeof(int)) + 1)) & 0x00FFFFFF) | 0xEA000000)

((sizeof(boot_file_head_t) + sizeof(int)) / sizeof(int))                                       -> Obtain the number of "words" occupied by the file header
(((sizeof(boot_file_head_t) + sizeof(int)) / sizeof(int)) + 1)                                 -> Set the offset to the real start code
(((((sizeof(boot_file_head_t) + sizeof(int)) / sizeof(int)) + 1)) & 0x00FFFFFF)                -> Obtain the signed-immed-24
((((((sizeof(boot_file_head_t) + sizeof(int)) / sizeof(int)) + 1)) & 0x00FFFFFF) | 0xEA000000) -> Assemble into a B instruction
```

#### About the jump_instruction for RISC-V

The jump_instruction field stores a jump instruction: (J  BACK_OF_boot_file_head). When this jump instruction is executed, the program will jump to the first instruction after the boot_file_head.

The Unconditional Jumps instruction encoding in RISC-V instructions is as follows:

```c
#define BROM_FILE_HEAD_SIZE (sizeof(boot_file_head_t) & 0x00FFFFF)
#define BROM_FILE_HEAD_BIT_10_1 ((BROM_FILE_HEAD_SIZE & 0x7FE) >> 1)
#define BROM_FILE_HEAD_BIT_11 ((BROM_FILE_HEAD_SIZE & 0x800) >> 11)
#define BROM_FILE_HEAD_BIT_19_12 ((BROM_FILE_HEAD_SIZE & 0xFF000) >> 12)
#define BROM_FILE_HEAD_BIT_20 ((BROM_FILE_HEAD_SIZE & 0x100000) >> 20)

#define BROM_FILE_HEAD_SIZE_OFFSET ((BROM_FILE_HEAD_BIT_20 << 31) |   \
                                    (BROM_FILE_HEAD_BIT_10_1 << 21) | \
                                    (BROM_FILE_HEAD_BIT_11 << 20) |   \
                                    (BROM_FILE_HEAD_BIT_19_12 << 12))

#define JUMP_INSTRUCTION (BROM_FILE_HEAD_SIZE_OFFSET | 0x6f)
```

This code appears to be a series of preprocessor macro definitions that manipulate the value of `BROM_FILE_HEAD_SIZE` to derive different bit positions and create a final constant `JUMP_INSTRUCTION`.

Unconditional Jumps

> The jump and link (JAL) instruction uses the J-type format, where the J-immediate encodes a signed offset in multiples of 2 bytes. The offset is sign-extended and added to the pc to form the jump target address. Jumps can therefore target a ±1 MiB range. JAL stores the address of the instruction following the jump (pc+4) into register rd. The standard software calling convention uses x1 as the return address register and x5 as an alternate link register.

Plain unconditional jumps (assembler pseudo-op J) are encoded as a JAL with rd=x0. we use it here.

The Plain unconditional jumps instruction in the RV (RISC-V) instruction set is a type of instruction that allows for unconditional branching in the program flow. It is represented by the assembler pseudo-op J. This instruction is used to unconditionally change the program's control flow and jump to a specified target address.

The encoding of the J instruction in the RV instruction set is as follows:
```
+------------------------------------------------+
|        imm[20]   |   imm[10:1]   |  imm[11]    |
+------------------------------------------------+
|      jump_offset[19:12]   |  rd (destination)  |
+------------------------------------------------+
|  opcode (6 bits) |   imm[19:12]   |     rd     |
+------------------------------------------------+
```

The J instruction takes a 20-bit signed immediate value and a target register (rd) as operands, where the target register stores the return address after the jump. The calculation of the target address for the jump is as follows:

1. Sign-extend the 20-bit immediate value to 32 bits.
2. Left-shift the immediate value by one bit (multiply by 2) to align it with the instruction boundary. The RV instruction set has a fixed instruction length of 4 bytes (32 bits), so the target address needs to be left-shifted by 1 bit for alignment.
3. Clear the lower bits of the PC register (current instruction address + 4) to align it with the instruction boundary.
4. Add the aligned PC address and the left-shifted immediate value to obtain the final jump target address.

Therefore, the J instruction in the RV instruction set is used for implementing unconditional jumps, allowing the program flow to be redirected to a specified target address, thereby altering the program's execution path.

Here's a brief description of each macro:
1. `BROM_FILE_HEAD_SIZE`: Calculates the size of `boot0_file_head_t` and masks it with `0x00FFFFF`.
2. `BROM_FILE_HEAD_BIT_10_1`: Extracts specific bits from `BROM_FILE_HEAD_SIZE` and shifts them.
3. `BROM_FILE_HEAD_BIT_11`: Extracts a specific bit from `BROM_FILE_HEAD_SIZE`.
4. `BROM_FILE_HEAD_BIT_19_12`: Extracts specific bits from `BROM_FILE_HEAD_SIZE` and shifts them.
5. `BROM_FILE_HEAD_BIT_20`: Extracts a specific bit from `BROM_FILE_HEAD_SIZE`.
6. `BROM_FILE_HEAD_SIZE_OFFSET`: Combines the extracted bits from previous macros to form a final offset value.
7. `JUMP_INSTRUCTION`: Creates a jump instruction using the calculated `BROM_FILE_HEAD_SIZE_OFFSET` and a constant value `0x6f` means `j #offset` which is `jal x0 #offset` in RV asm.

#### About the check_sum and length

In the block device, need to padding the binary file, and that will need to reset the checksum and length, we provide a tool `mksunxi.c` to adjusting bootloader files by correcting header information and ensuring correct formatting.

Specific operations include:
1. Reading command-line arguments, such as the bootloader file name and padding size.
2. Opening the bootloader file and obtaining its size.
3. Ensuring that the file size is greater than the size of the bootloader header.
4. Allocating memory and reading the contents of the bootloader file into memory.
5. Calculating the length of the bootloader and aligning it based on the padding size.
6. Computing the checksum and updating the checksum field in the header information.
7. Writing the corrected bootloader back to the file.
8. Displaying information about the corrected bootloader header and the bootloader size.

You can find this tool in `tools/mksunxi.c`

