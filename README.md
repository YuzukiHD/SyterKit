# SyterKit
![syterkit-high-resolution-logo-transparent](https://github.com/YuzukiHD/SyterKit/assets/12003087/d010425a-d93e-4ecf-9304-c56414c18123)

SyterKit is a bare-metal framework designed for Allwinner platform. SyterKit utilizes CMake as its build system and supports various applications and peripheral drivers. Additionally, SyterKit also has bootloader functionality, which enables it to replace U-Boot for fast booting (standard Linux 6.7 mainline boot time of 1.02s, significantly faster than traditional U-Boot's 3s boot time).

## Support list

| Board                                                        | Manufacturer | Platform | Spec                              | Details                                        | Config                  |
| ------------------------------------------------------------ | ------------ | -------- | --------------------------------- | ---------------------------------------------- | ----------------------- |
| [Yuzukilizard](https://github.com/YuzukiHD/Yuzukilizard)     | YuzukiHD     | V851s    | Cortex A7                         | [board/yuzukilizard](board/yuzukilizard)       | `yuzukilizard.cmake`    |
| [TinyVision](https://github.com/YuzukiHD/TinyVision)         | YuzukiHD     | V851se   | Cortex A7                         | [board/tinyvision](board/tinyvision)           | `tinyvision.cmake`      |
| 100ask-t113s3                                                | 100ask       | T113-S3  | Dual-Core Cortex A7               | [board/100ask-t113s3](board/100ask-t113s3)     | `100ask-t113s3.cmake`   |
| 100ask-t113i                                                 | 100ask       | T113-I   | Dual-Core Cortex A7 + C906 RISC-V | [board/100ask-t113i](board/100ask-t113i)       | `100ask-t113i.cmake`    |
| dongshanpi-aict                                              | 100ask       | V853     | Cortex A7                         | [board/dongshanpi-aict](board/dongshanpi-aict) | `dongshanpi-aict.cmake` |
| project-yosemite                                             | YuzukiHD     | V853     | Cortex A7                         | [board/project-yosemite](board/project-yosemite) | `project-yosemite.cmake` |
| 100ask ROS                                                   | 100ask       | R818     | Quad-Core Cortex A53              | [board/100ask-ros](board/100ask-ros)           | `100ask-ros.cmake`      |
| [longanpi-3h](https://wiki.sipeed.com/hardware/zh/longan/H618/lpi3h/1_intro.html) | sipeed       | H618     | Quad-Core Cortex A53              | [board/longanpi-3h](board/longanpi-3h)         | `longanpi-3h.cmake`     |
| longanpi-4b                                                  | sipeed       | T527     | Octa-Core Cortex A55              | [board/longanpi-4b](board/longanpi-4b)         | `longanpi-4b.cmake`     |
| BingPi-M1                                                    | BingPi       | V3s      | Cortex A7                         | [board/bingpi-m1](board/bingpi-m1)             | `bingpi-m1.cmake`       |

## Getting Started

### Building SyterKit From Scratch

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

### Creating TF Card Boot Firmware

After build the firmware, you can flash it into the TF card. For the V851s platform, you can write it to either an 8K offset or a 128K offset. Generally, if the TF card uses MBR format, write it with an 8K offset. If it uses GPT format, write it with a 128K offset. Assuming `/dev/sdb` is the target TF card, you can use the following command to write it with an 8K offset:

```shell
sudo dd if=syter_boot_bin_card.bin of=/dev/sdb bs=1024 seek=8
```

If it is a GPT partition table, you need to write it with a 128K offset:

```shell
sudo dd if=syter_boot_bin_card.bin of=/dev/sdb bs=1024 seek=128
```

#### Creating the Firmware for SPI NAND

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

#### Creating the Firmware for SPI NOR

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
