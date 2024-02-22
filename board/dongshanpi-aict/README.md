# DongshanPI-AICT

## Specifications

![100ask-v853-pro_top](https://github.com/YuzukiHD/SyterKit/assets/12003087/0240756e-d8cc-4240-9486-6ba3b2d55d56)


- Based on Allwinner V853
- Cortex-A7 Core up to 1200MHz 
- RISC-V E907GC@600MHz
- 1Tops@int8 NPU
- Built in 512MB/1GB DDR3  memory
- One TF Card Slot, Support UHS-SDR104
- On board SPI NAND /  8/32G Emmc
- On board USB&UART Combo
- Supports one 2/4-lane MIPI CSI inputs 
- Supports 1 individual ISP, with maximum resolution of 2560 x 1440
- H.264/H.265 decoding at 4096x4096
- H.264/H.265 encoder supports 3840x2160@20fps
- Online Video encode
- RISC-V E907 RTOS Support, Based on RT-Thread + RTOS-HAL

## Application

| Name            | Function                                                     | Path              |
| --------------- | ------------------------------------------------------------ | ----------------- |
| hello world     | Minimal program example, prints Hello World                  | `hello_world`     |
| init dram       | Initializes the serial port and DRAM                         | `init_dram`       |
| read chip efuse | Reads chip efuse information                                 | `read_chip_efuse` |
| read chipsid    | Reads the unique ID of the chip                              | `read_chipsid`    |
| load e907       | Reads the e907 core firmware, starts the e907 core, and uses V851s as a large RISC-V microcontroller (E907 @ 600 MHz with 64MB memory) | `load_e907`       |
| syter boot      | Bootstrapping function that replaces U-Boot, enabling fast system startup for Linux | `syter_boot`      |
| syter boot_spi  | Bootstrapping function that replaces U-Boot, enabling fast system startup for Linux for SPI NAND, SPI NOR | `syter_boot_spi`  |
| syter amp       | Reads the e907 core firmware, starts the e907 core, loads the kernel, and runs Linux simultaneously on both e907 and a7 systems, which are heterogeneously integrated | `syter_amp`       |
| fdt parser      | Reads the DTB and Parser Print out                           | `fdt_parser`      |
| fdt cli         | Reads the DTB with a CLI support uboot fdt command           | `fdt_cli`         |
| syter bootargs  | Bootstrapping function that replaces U-Boot, enabling fast system startup for Linux, Within a CLI to change bootargs | `syter_bootargs`  |
| cli test        | Test baisc CLI function                                      | `cli_test`        |
| i2c oled        |                                                              | `i2c_oled`        |
| i2c test        |                                                              | `i2c_test`        |
| spi lcd         |                                                              | `spi_lcd`         |
| syter boot spi  |                                                              | `syter_boot_spi`  |

## Buy Now

### DongshanPI-AICT

- Taobao:  https://item.taobao.com/item.htm?id=706864521673

### Accessories

- MIPI GC2053 Camera: [https://item.taobao.com/item.htm?id=706864521673&skuId=5213471339193](https://item.taobao.com/item.htm?id=706864521673&skuId=5213471339193)

- MIPI DSI 800x480 Display:  https://item.taobao.com/item.htm?id=706091265930

  
