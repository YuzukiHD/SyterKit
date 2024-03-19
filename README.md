# SyterKit

![SyterKit LOGO_Thin](https://github.com/YuzukiHD/SyterKit/assets/12003087/e6135860-1a6a-4cb4-b0f6-71af8eca1509)

SyterKit is a bare-metal framework designed for Allwinner platform. SyterKit utilizes CMake as its build system and supports various applications and peripheral drivers. Additionally, SyterKit also has bootloader functionality

## Support list

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
| [LT527X](https://www.myir.cn/shows/134/70.html)              | myir-tech    | T527 | Octa-Core Cortex A55 | [board/lt527x](board/lt527x) | `lt527x.cmake` |

