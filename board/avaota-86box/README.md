# T113i-Industrial DevKit

## Specifications

- Main control: Allwinner T113i   Dual-core ARM Cortex -A7 + 64-bit Xuantie C906 RISC-V
- DRAM: DDR3 512MB
- Storage: Onboard 128MB spi-nand, support USB external U disk and SD card to expand storage
- Debug: Onboard TypeC usb to serial ttl debug
- Network:  Support Gigabit Ethernet,  Support USB 100M Ethernet,Support 2.4G/5.8G WiFi and Bluetooth, onboard antenna
- Display: Support RGB+TP screen interface,  support SPI screen
- Audio:  3.5mm headphone jack * 1 
- Power: 12V/2A  DC-5.5/2.1
- Connectivity: CANx2 RS485 x 6
- Pinout: 40Pin x2.54 IO
- Board size: length 125mm *width 125mm* thickness 1.7mm
- PCB layer: 4+4 layers
- Support Tina Linux，based on Linux 5.4 kernel



## Application

| Name        | Function                                                     | Path             |
| ----------- | ------------------------------------------------------------ | ---------------- |
| sys_info    | The system initializes necessary functions and prints hello word | `sys_info`       |
| hello world | Minimal program example, prints Hello World                  | `hello_world`    |
| load c906   | Start and initialize the tf card through the C906 CPU, read the hifi4 dsp firmware from it, and load it for execution | `load_c906`      |
| syter boot  | Bootstrapping function that replaces U-Boot, enabling fast system startup for Linux | `app/syter_boot` |
| os test     | The system initializes the serial port information and prints hello word to verify whether the system starts normally. | `os_test`        |

## Buy Now

### T113i-Industrial

Tabao:  https://item.taobao.com/item.htm?id=756032410469

### Accessories

RGB-1024x600: https://item.taobao.com/item.htm?id=611156659477&



