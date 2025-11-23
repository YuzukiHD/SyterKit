# SyterKit

![SyterKit LOGO_Thin](https://github.com/YuzukiHD/SyterKit/assets/12003087/e6135860-1a6a-4cb4-b0f6-71af8eca1509)

SyterKit is a bare-metal framework designed for Allwinner platform. SyterKit utilizes CMake as its build system and supports various applications and peripheral drivers. Additionally, SyterKit also has bootloader functionality

## Support list

| Board                                                        | Platform      | Spec                                          | Config                   |
| ------------------------------------------------------------ | ------------- | --------------------------------------------- | ------------------------ |
| [Yuzukilizard](https://github.com/YuzukiHD/Yuzukilizard)     | V851s         | Cortex A7                                     | `yuzukilizard.cmake`     |
| [TinyVision](https://github.com/YuzukiHD/TinyVision)         | V851se        | Cortex A7                                     | `tinyvision.cmake`       |
| 100ask-t113s3                                                | T113-S3       | Dual-Core Cortex A7                           | `100ask-t113s3.cmake`    |
| 100ask-t113i                                                 | T113-I        | Dual-Core Cortex A7 + C906 RISC-V             | `100ask-t113i.cmake`     |
| 100ask-d1-h                                                  | D1-H          | C906 RISC-V                                   | `100ask-d1-h.cmake`      |
| dongshanpi-aict                                              | V853          | Cortex A7                                     | `dongshanpi-aict.cmake`  |
| project-yosemite                                             | V853          | Cortex A7                                     | `project-yosemite.cmake` |
| mCore-R818                                                   | R818          | Quad-Core Cortex A53                          | `mcore-r818.cmake`       |
| [longanpi-3h](https://wiki.sipeed.com/hardware/zh/longan/H618/lpi3h/1_intro.html) | H618          | Quad-Core Cortex A53                          | `longanpi-3h.cmake`      |
| Avaota A1                                                    | T527/A527     | Octa-Core Cortex A55                          | `avaota-a1.cmake`        |
| Radxa Cubie A7A                                              | A733          | Dual-Core Cortex A76 + Hexa-Core Cortex A55   | `radxa-cubie-a7a.cmake`  |
| Avaota F1                                                    | V821          | RISC-V RV32 CPU + RISC-V RV32 MCU             | `avaota-f1.cmake`        |
| TLT536-EVM                                                   | T536          | Quad-Core Cortex A55                          | `tlt536-evm.cmake`       |
| Yuzukihomekit                                                | T113-M4020DC0 | Dual-Core Cortex A7 + C906 RISC-V + HIFI4 DSP | `yuzukihomekit.cmake`    |

# Getting Started

## SyterKit Architecture
![SyterKit_Arch](https://github.com/YuzukiHD/SyterKit/assets/12003087/f6ffe47e-6274-43ff-9a74-4a5b7b81083e)

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


## License
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2FYuzukiHD%2FSyterKit.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2FYuzukiHD%2FSyterKit?ref=badge_large)
