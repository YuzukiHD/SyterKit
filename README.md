# SyterKit

- [中文版](#syterkit-1)

SyterKit is a bare-metal framework designed for development boards like TinyVision or other chips such as v851se/v851s/v851s3/v853. SyterKit utilizes CMake as its build system and supports various applications and peripheral drivers. Additionally, SyterKit also has bootloader functionality, which enables it to replace U-Boot for fast booting (standard Linux 6.7 mainline boot time of 1.02s, significantly faster than traditional U-Boot's 3s boot time).

## App

| Name            | Function                                                     | Path                  |
| --------------- | ------------------------------------------------------------ | --------------------- |
| hello world     | Minimal program example, prints Hello World                  | `app/hello_world`     |
| init dram       | Initializes the serial port and DRAM                         | `app/init_dram`       |
| read chip efuse | Reads chip efuse information                                 | `app/read_chip_efuse` |
| read chipsid    | Reads the unique ID of the chip                              | `app/read_chipsid`    |
| load e907       | Reads the e907 core firmware, starts the e907 core, and uses V851s as a large RISC-V microcontroller (E907 @ 600 MHz with 64MB memory) | `app/load_e907`       |
| syter boot      | Bootstrapping function that replaces U-Boot, enabling fast system startup for Linux | `app/syter_boot`      |
| syter amp       | Reads the e907 core firmware, starts the e907 core, loads the kernel, and runs Linux simultaneously on both e907 and a7 systems, which are heterogeneously integrated | `app/syter_amp`       |
| fdt parser      | Reads the DTB and Parser Print out                           | `app/fdt_parser`      |
| fdt cli         | Reads the DTB with a CLI support uboot fdt command           | `app/fdt_cli`         |
| syter bootargs  | Bootstrapping function that replaces U-Boot, enabling fast system startup for Linux, Within a CLI to change bootargs | `app/syter_bootargs`  |
| cli test        | Test baisc CLI function                                      | `app/cli_test`        |

## Getting Started

### Building SyterKit From Scratch

Building SyterKit is a straightforward process that only requires setting up the environment for compilation on GNU/Linux, Windows or macOS(untested). The software packages required by SyterKit include:

- `gcc-arm-none-eabi` (`gcc-linaro-arm-eabi` works as well)
- `CMake`
- `GNU Make`

#### Prepare Building Environment on GNU/Linux
For commonly used Ubuntu systems, they can be installed using the following command:

```shell
sudo apt-get update
sudo apt-get install gcc-arm-none-eabi cmake build-essential -y
```

#### Prepare Building Environment on Windows
On Windows, the packages are required to be installed by hand:
- [gcc-arm-eabi](https://releases.linaro.org/components/toolchain/binaries)
- [CMake](https://cmake.org/download/)
- [GNU Make](https://gnuwin32.sourceforge.net/packages/make.htm)

*Note: it's a good idea to add the bin directories of above into `PATH` variable to avoid trouble.*

#### Start building
Create a folder to store the compiled output files and navigate to it:

```shell
mkdir build
cd build
```

Run the following commands to configure CMake cache:

```shell
cmake ..
```

If you are using Windows, you may need to specify the toolchain path in [cmake\toolchain-arm-eabi.cmake](cmake/toolchain-arm-eabi.cmake#L26). Then you can configure CMake cache by running

```shell
cmake .. -G 'Unix Makefiles'
```

*Note: `-G 'Unix Makefiles' ` may be not required, add it when CMake is trying to generate a Visual Studio project or something else.*

Run the following commands to build SyterKit
```shell
cd build
cmake --build . # You can add "-j" to enable multi-thread compilation
```


The compiled executable files will be located in `build/app`.

![image-20231206212123866](assets/post/README/image-20231206212123866.png)

The SyterKit project will compile two versions: firmware ending with `.elf` is for USB booting and requires bootloading by PC-side software, while firmware ending with `.bin` is for flashing and can be written into storage devices such as TF cards and SPI NAND.

- For SD Card, You need to flash the `xxx_card.bin`
- For SPI NAND/SPI NOR, You need to flash the `xxx_spi.bin`

To boot from USB or flash the boot image into SPI flash chips, you need to install the [XFEL](https://xboot.org/xfel) tool.

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

### Booting from USB
Allwinner SoCs support `FEL` mode, which enables them to boot from USB, which is a convenient way to test SyterKit. Make sure you have installed XFEL tool,then you can load the fel image into the chip's memory and boot it:

For example, to boot the `hello_world` firmware, you can use the following 

```shell
xfel write 0x28000 syter_boot_elf.bin # load the firmware into memory, 0x28000 is the beginning address of the SRAM of Allwinner V85x.

xfel exec 0x28000 # boot the firmware
```
Then you can see the output of the firmware from UART0.

# SyterKit

SyterKit 是一个纯裸机框架，用于 TinyVision 或者其他 v851se/v851s/v851s3/v853 等芯片的开发板，SyterKit 使用 CMake 作为构建系统构建，支持多种应用与多种外设驱动。同时 SyterKit 也具有启动引导的功能，可以替代 U-Boot 实现快速启动（标准 Linux6.7 主线启动时间 1.02s，相较于传统 U-Boot 启动快 3s）。

## 应用

| 名称            | 功能                                                         | 路径                  |
| --------------- | ------------------------------------------------------------ | --------------------- |
| hello world     | 最小程序示例，打印 Hello World                               | `app/hello_world`     |
| init dram       | 初始化串行端口和 DRAM                                        | `app/init_dram`       |
| read chip efuse | 读取芯片 efuse 信息                                          | `app/read_chip_efuse` |
| read chipsid    | 读取芯片的唯一 ID                                            | `app/read_chipsid`    |
| load e907       | 读取 e907 核心固件，启动 e907 核心，并使用 V851s 作为大型 RISC-V 微控制器（E907 @ 600 MHz，64MB 内存） | `app/load_e907`       |
| syter boot      | 替代 U-Boot 的引导函数，为 Linux 启用快速系统启动            | `app/syter_boot`      |
| syter amp       | 读取 e907 核心固件，启动 e907 核心，加载内核，并在 e907 和 a7 系统上同时运行 Linux，系统是异构集成运行的 | `app/syter_amp`       |
| fdt parser      | 读取设备树二进制文件并解析打印输出                           | `app/fdt_parser`      |
| fdt cli         | 使用支持 uboot fdt 命令的 CLI 读取设备树二进制文件           | `app/fdt_cli`         |
| syter bootargs  | 替代 U-Boot 引导，为 Linux 启用快速系统启动，支持在 CLI 中更改启动参数 | `app/syter_bootargs`  |
| cli test        | 测试基本 CLI 功能                                            | `app/cli_test`        |

## 开始使用

### 从零构建 SyterKit 

构建 SyterKit 非常简单，只需要在 GNU/Linux, Windows或macOS(未测试) 操作系统中安装配置环境即可编译。SyterKit 需要的软件包有：

- `gcc-arm-none-eabi`
- `CMake`
- `GNU Make`

#### 在 GNU/Linux 上准备构建环境
对于常用的 Ubuntu 系统，可以通过如下命令安装

```shell
sudo apt-get update
sudo apt-get install gcc-arm-none-eabi cmake build-essential -y
```

#### 在 Windows 上准备构建环境
在 Windows 上，需要手动安装软件包：
- [gcc-arm-eabi](https://releases.linaro.org/components/toolchain/binaries)
- [CMake](https://cmake.org/download/)
- [GNU Make](https://gnuwin32.sourceforge.net/packages/make.htm)

*注意：建议将上述软件包的 bin 目录添加到 `PATH` 环境变量中，以免出现麻烦。*

#### 开始构建

新建一个文件夹存放编译的输出文件，并且进入这个文件夹

```shell
mkdir build
cd build
```

运行命令编译 SyterKit

```shell
cmake ..
```

如果你使用的是 Windows，你可能需要在 [cmake\toolchain-arm-eabi.cmake](cmake/toolchain-arm-eabi.cmake#L26) 中指定工具链路径。然后你可以运行以下命令配置 CMake 缓存

```shell
cmake .. -G 'Unix Makefiles'
```

*注意：`-G 'Unix Makefiles'` 可能不是必须的，当 CMake 尝试生成 Visual Studio 项目或其他项目时添加它。*


运行命令构建 SyterKit

```shell
cd build
cmake --build . # 可以添加 "-j" 开启多线程编译
```

编译后的可执行文件位于 `build/app` 中

![image-20231206212123866](assets/post/README/image-20231206212123866.png)

同一个项目工程 SyterKit 会编译两个版本，`elf` 结尾的固件为 USB 引导固件，需要电脑上位软件进行引导加载，`bin` 结尾的固件为刷写固件，可以刷入 TF 卡，SPI NAND 等储存器。

- 对于 SD 卡，你需要刷写 `xxx_card.bin` 文件。
- 对于 SPI NAND/SPI NOR，你需要刷写 `xxx_spi.bin` 文件。

要通过 USB 引导或者刷写 SPI Flash 的固件，你需要安装 [XFEL](https://xboot.org/xfel) 工具。

### 制作 TF 卡启动固件

固件可以刷入TF卡内使用，对于 V851s 平台，可以将其写入 8K 偏移位或者 128K 偏移位。一般来说如果TF卡使用的是 MBR 格式，写 8K 偏移，如果是 GPT 格式，写128K 偏移。这里假设 `/dev/sdb` 是目标 TF 卡。写入 8K 偏移位

```shell
sudo dd if=syter_boot_bin_card.bin of=/dev/sdb bs=1024 seek=8
```

如果是 GPT 分区表，需要写入 128K 偏移量

```shell
sudo dd if=syter_boot_bin_card.bin of=/dev/sdb bs=1024 seek=128
```

#### 制作 SPI NAND 启动固件

制作 SPI NAND 的固件，需要将 SyterKit 写入对应位置制作固件

```shell
dd if=syter_boot_bin_spi.bin of=spi.img bs=2k
dd if=syter_boot_bin_spi.bin of=spi.img bs=2k seek=32
dd if=syter_boot_bin_spi.bin of=spi.img bs=2k seek=64
```

同时也可以把 Linux 内核与设备树一同写入固件中

```shell
dd if=sunxi.dtb of=spi.img bs=2k seek=128     # DTB on page 128
dd if=zImage of=spi.img bs=2k seek=256        # Kernel on page 256
```

制作完成的固件可以使用 xfel 工具刷入 SPI NAND

```shell
xfel spinand write 0x0 spi.img
```

#### 制作 SPI NOR 启动固件

制作 SPI NOR 的固件，需要将 SyterKit 写入对应位置制作固件

```shell
dd if=syter_boot_bin_spi.bin of=spi.img bs=2k
dd if=syter_boot_bin_spi.bin of=spi.img bs=2k seek=32
dd if=syter_boot_bin_spi.bin of=spi.img bs=2k seek=64
```

同时也可以把 Linux 内核与设备树一同写入固件中

```shell
dd if=sunxi.dtb of=spi.img bs=2k seek=128     # DTB on page 128
dd if=zImage of=spi.img bs=2k seek=256        # Kernel on page 256
```

制作完成的固件可以使用 xfel 工具刷入 SPI NOR

```shell
xfel spinor write 0x0 spi.img
```

### 通过 USB 引导

Allwinner SoC 支持 `FEL` 模式，可以通过 USB 引导，这是一个方便的测试 SyterKit 的方式。确保你已经安装了 XFEL 工具，然后可以将 fel 镜像加载到芯片的内存中并启动它。

例如，要引导 `hello_world` 固件，可以使用以下命令

```shell
xfel write 0x28000 syter_boot_elf.bin # 把固件加载到内存中，0x28000 是 Allwinner V85x 的 SRAM 的起始地址。

xfel exec 0x28000 # 启动固件
```
