set(USB_DRIVER "")
if (CONFIG_CHIP_USB)
set(USB_DRIVER
    usb/usb.c
    usb/usb_controller.c
    usb/usb_device.c
    usb/usb_dma.c

    usb/module/usb_detect.c
    usb/module/usb_mass.c
)
endif()