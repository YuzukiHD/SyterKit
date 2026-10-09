# Display driver (TCON + DE UI passthrough)

`drivers/display` drives the pipeline `DE (one UI layer) -> TCON -> RGB | LVDS | DSI -> panel + backlight`.
The display engine does no composition: layer 0 of the UI channel goes straight to the TCON.

## Layout

| Path | Content |
| --- | --- |
| `display.c`, `display-clk.c` | sequencing; bus gate/reset, module clock and PLL_VIDEO/PLL_PERI helpers |
| `tcon/` | TCON LCD (HV, LVDS, DSI trigger mode) and TCON top, chip independent code |
| `de/`, `dsi/`, `dphy/` | UI passthrough, DSI host, combo D-PHY, chip independent code |
| `panel/`, `backlight/` | panel command sequences (GPIO, delay, DCS/generic DSI writes), GPIO/PWM backlight |
| `platform/<soc>/` | **everything that differs between SoCs**, one directory per SoC (`soc.c tcon.c de.c dsi.c dphy.c`): register offsets and field layouts, clock trees, PLL fields, fused trims |

Each block selects its description at run time from the `allwinner,variant` string of its node; a new SoC adds
`platform/<soc>/<block>.c` (plus an entry in the lookup table of the block) and does not touch the common code.

## Device tree

`dts/bindings/display/*.yaml`, readers in `dts/include/dt-compatible/{display,de,tcon,dsi,dphy}-dt.h`,
example nodes in `boards/yuzukineko/board.dts` (`display0`). The `allwinner,output` phandle of the
`allwinner,sunxi-display` node picks `allwinner,sunxi-rgb`, `allwinner,sunxi-lvds` or `allwinner,sunxi-mipi-dsi`.
Command sequence macros: `dt-bindings/display/sunxi-display.h`.

## Use

```c
sunxi_display_dt_read_alias(&disp, "display0");
sunxi_display_init(&disp);
sunxi_display_set_fb(&disp, fb, w, h, stride, SUNXI_DE_FMT_XRGB8888);   /* flush the cache first */
sunxi_display_enable(&disp);
```

`sunxi_display_set_pattern()` shows a TCON test pattern without the DE. Sample: `boards/yuzukineko/app_sram/disp_test`.

## Status

Verified on the YuzukiNeko board (JD9168S DSI panel, 1024x768): TCON colour bar and a DE framebuffer (colour bars) are shown.
Build: `make yuzukineko_rv32_sram_defconfig` (with `CONFIG_DRIVER_DISPLAY=y`), `make disp_test`, then `xfel write 0x20000 disp_test_fel.bin`
and `xfel exec 0x20000` from FEL. (`rv32_dram_defconfig` builds the same test as a PSRAM image: `xfel ddr f101-s3`, write/exec at 0x40100000.)

Things learned on the way (all handled in the code):

- The display engine's register file lives in the SRAM above 0x2b000; `platform/<soc>/soc.c` hands it to the DE, so an SRAM image
  must stay below 0x2b000 (code, bss and stack) and keep its big state and the stack in the PSRAM (`disp_test` switches the stack).
- A DSI panel needs the TCON pixel polarity left alone (the DSI host owns the data path).
- LP command transfers drop the D-PHY clock lane out of HS, so the video start uses the `HS_VIDEO` sequence (clock + data lanes).
- The PHY/DSI/TCON module clocks must select their parent and start PLL_VIDEO; the combo-phy clock is a fixed /4 (`allwinner,module-div`).
- The pin controller of this SoC has 4 drive-strength bits per pin; gpio-v2 `sunxi_gpio_set_drv` was writing 2 bits per pin and is fixed to 4.
- Backlight polarity follows the SyterKit PWM driver (`allwinner,active-high = <1>` on this board).
- Panel BIST (`C2 30` after `DE 00` for the JD9168S) is a quick way to tell panel/reset problems from video link problems.

RGB: the TCON and TCON top registers (and the pixel clock plan) match a working reference run bit for bit with the same panel timing
(no panel attached on the bench board, so the picture itself was not seen). The TCON pad select (GCTL bit 1) is set for DSI only.

Not done: a second SoC variant (needs the register reference of the target chip), LVDS and command-mode DSI are untested.
