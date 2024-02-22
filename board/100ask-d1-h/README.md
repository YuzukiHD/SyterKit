# 100ASK D1-H Dual Display DevKit

## Specifications

![D1h-DualDisplay-Devkit_top](https://github.com/YuzukiHD/SyterKit/assets/12003087/00975cce-f95d-4706-9042-fc80486c1c75)


- Main control: Allwinner D1-H C906 RISC-V 1GHz
- DRAM: DDR3 512MB
- Storage: Onboard 128MB spi-nand, support USB external U disk and SD card to expand storage
- Network: Support 2.4G WiFi and Bluetooth, onboard antenna
- Display: Support MIPI-DSI+TP screen interface, support HDMI output, support SPI screen
- Audio: Microphone daughter board interface * 1, 3.5mm headphone jack * 1 
- Board size: length 90mm *width 105mm* thickness 1.7mm
- PCB layer: 4+4 layers
- Support Tina Linux，based on Linux 5.4 kernel

## Application

| Name        | Function                                                     | Path              |
| ----------- | ------------------------------------------------------------ | ----------------- |
| hello world | Minimal program example, prints Hello World                  | `app/hello_world` |
| init dram   | Initializes the serial port and DRAM                         | `app/init_dram`   |
| load hifi4  | Start and initialize the tf card through the C906 CPU, read the hifi4 dsp firmware from it, and load it for execution | `app/load_hifi4`  |
| syter boot  | Bootstrapping function that replaces U-Boot, enabling fast system startup for Linux | `app/syter_boot`  |

## Buy Now

### DualDisplay DevKit

Tabao: [https://item.taobao.com/item.htm?id=761073687978](https://item.taobao.com/item.htm?id=761073687978)

### Accessories

NezhaCore: [https://item.taobao.com/item.htm?id=761073687978&skuId=5247201813557](https://item.taobao.com/item.htm?id=761073687978&skuId=5247201813557)



