# TinyVision

TinyVision - A Tiny Linux Board / IPC / Server / Router / And so on...

![image](https://github.com/YuzukiHD/SyterKit/assets/12003087/14378d81-ae4d-4008-b74d-abfc4c0ca6ac)

- Based on Allwinner V851se / V851s3 
- Cortex-A7 Core up to 1200MHz 
- RISC-V E907GC@600MHz
- 0.5Tops@int8 NPU
- Built in 64M DDR2 (V851se) / 128M DDR3L (V851s3) memory
- One TF Card Slot, Support UHS-SDR104
- On board SD NAND
- On board USB&UART Combo
- Supports one 2-lane MIPI CSI inputs
- Supports 1 individual ISP, with maximum resolution of 2560 x 1440
- H.264/H.265 decoding at 4096x4096
- H.264/H.265 encoder supports 3840x2160@20fps
- Online Video encode
- RISC-V E907 RTOS Support, Based on RT-Thread + RTOS-HAL

## App

| Name            | Function                                                     | Path                  |
| --------------- | ------------------------------------------------------------ | --------------------- |
| hello world     | Minimal program example, prints Hello World                  | `app/hello_world`     |
| init dram       | Initializes the serial port and DRAM                         | `app/init_dram`       |
| read chip efuse | Reads chip efuse information                                 | `app/read_chip_efuse` |
| read chipsid    | Reads the unique ID of the chip                              | `app/read_chipsid`    |
| load e907       | Reads the e907 core firmware, starts the e907 core, and uses V851s as a large RISC-V microcontroller (E907 @ 600 MHz with 64MB memory) | `app/load_e907`       |
| syter boot      | Bootstrapping function that replaces U-Boot, enabling fast system startup for Linux | `app/syter_boot`      |
| syter boot_spi  | Bootstrapping function that replaces U-Boot, enabling fast system startup for Linux for SPI NAND, SPI NOR| `app/syter_boot_spi`      |
| syter amp       | Reads the e907 core firmware, starts the e907 core, loads the kernel, and runs Linux simultaneously on both e907 and a7 systems, which are heterogeneously integrated | `app/syter_amp`       |
| fdt parser      | Reads the DTB and Parser Print out                           | `app/fdt_parser`      |
| fdt cli         | Reads the DTB with a CLI support uboot fdt command           | `app/fdt_cli`         |
| syter bootargs  | Bootstrapping function that replaces U-Boot, enabling fast system startup for Linux, Within a CLI to change bootargs | `app/syter_bootargs`  |
| cli test        | Test baisc CLI function                                      | `app/cli_test`        |

## 购买链接

### TinyVision

- [https://item.taobao.com/item.htm?&id=756255119524](https://item.taobao.com/item.htm?&id=756255119524)

### 配套配件

- GC2053摄像头: [https://item.taobao.com/item.htm?&id=736796459015](https://item.taobao.com/item.htm?&id=736796459015)
- RJ45 百兆线（选择4P转水晶头 50CM）: [https://item.taobao.com/item.htm?&id=626832235333](https://item.taobao.com/item.htm?&id=626832235333)
