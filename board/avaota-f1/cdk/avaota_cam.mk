##
## Auto Generated makefile by CDK
## Do not modify this file, and any manual changes will be erased!!!   
##
## BuildSet
ProjectName            :=avaota_cam
ConfigurationName      :=BuildSet
WorkspacePath          :=./
ProjectPath            :=./
IntermediateDirectory  :=Obj
OutDir                 :=$(IntermediateDirectory)
User                   :=zhangyuanjing
Date                   :=03/12/2024
CDKPath                :=../../../../../C-Sky/CDK
ToolchainPath          :=D:/C-Sky/CDKRepo/Toolchain/XTGccElfNewlib/V2.6.1/R/
LinkerName             :=riscv64-unknown-elf-gcc
LinkerNameoption       :=
SIZE                   :=riscv64-unknown-elf-size
READELF                :=riscv64-unknown-elf-readelf
CHECKSUM               :=crc32
SharedObjectLinkerName :=
ObjectSuffix           :=.o
DependSuffix           :=.d
PreprocessSuffix       :=.i
DisassemSuffix         :=.asm
IHexSuffix             :=.ihex
BinSuffix              :=.bin
ExeSuffix              :=.elf
LibSuffix              :=.a
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
ElfInfoSwitch          :=-hlS
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
UnPreprocessorSwitch   :=-U
SourceSwitch           :=-c 
ObjdumpSwitch          :=-S
ObjcopySwitch          :=-O ihex
ObjcopyBinSwitch       :=-O binary
OutputFile             :=$(ProjectName)
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
PreprocessOnlyDisableLineSwitch   :=-P
ObjectsFileList        :=avaota_cam.txt
MakeDirCommand         :=mkdir
LinkOptions            := -mcpu=e907f  --specs=nosys.specs  -nostartfiles -Wl,--gc-sections -T"$(ProjectPath)/link_elf.ld"
LinkOtherFlagsOption   := -Wl,-Map=$(ProjectPath)/Lst/$(OutputFile).map 
IncludePackagePath     :=
IncludeCPath           := $(IncludeSwitch). $(IncludeSwitch)../../../include/ $(IncludeSwitch)../../../include/arch/riscv/ $(IncludeSwitch)../../../include/cli/ $(IncludeSwitch)../../../include/drivers/chips/sun20iw5/ $(IncludeSwitch)../../../include/drivers/mmc/ $(IncludeSwitch)../../../include/drivers/pmu/reg/ $(IncludeSwitch)../../../include/drivers/pmu/ $(IncludeSwitch)../../../include/drivers/reg/ $(IncludeSwitch)../../../include/drivers/usb/ $(IncludeSwitch)../../../include/image/ $(IncludeSwitch)../../../include/lib/elf/ $(IncludeSwitch)../../../include/lib/fatfs/ $(IncludeSwitch)../../../include/lib/fdt/ $(IncludeSwitch)../../../include/drivers/ $(IncludeSwitch)../../../include/drivers/chips/ $(IncludeSwitch)../../../include/drivers/mtd  
IncludeAPath           := $(IncludeSwitch). $(IncludeSwitch)../../../include/ $(IncludeSwitch)../../../include/arch/riscv/ $(IncludeSwitch)../../../include/cli/ $(IncludeSwitch)../../../include/drivers/chips/sun20iw5/ $(IncludeSwitch)../../../include/drivers/mmc/ $(IncludeSwitch)../../../include/drivers/pmu/reg/ $(IncludeSwitch)../../../include/drivers/pmu/ $(IncludeSwitch)../../../include/drivers/reg/ $(IncludeSwitch)../../../include/drivers/usb/ $(IncludeSwitch)../../../include/image/ $(IncludeSwitch)../../../include/lib/elf/ $(IncludeSwitch)../../../include/lib/fatfs/ $(IncludeSwitch)../../../include/lib/fdt/ $(IncludeSwitch)../../../include/drivers/ $(IncludeSwitch)../../../include/drivers/chips/  
Libs                   :=
ArLibs                 := 
PackagesLibPath        :=
LibPath                := $(PackagesLibPath) 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       :=riscv64-unknown-elf-ar rcu
CXX      :=riscv64-unknown-elf-g++
CC       :=riscv64-unknown-elf-gcc
AS       :=riscv64-unknown-elf-gcc
OBJDUMP  :=riscv64-unknown-elf-objdump
OBJCOPY  :=riscv64-unknown-elf-objcopy
CXXFLAGS := -mcpu=e907f   $(PreprocessorSwitch)CONFIG_CHIP_SUN20IW5  $(PreprocessorSwitch)DEBUG_MODE   -O0  -g3  -Wall  -ffunction-sections  -fdata-sections -nostdlib -nostdinc -march=rv32imafcxthead -mabi=ilp32f -Wno-int-to-pointer-cast -Wno-int-to-pointer-cast -Wno-shift-count-overflow -Wno-builtin-declaration-mismatch -Wno-pointer-to-int-cast -Wno-implicit-function-declaration -Wno-discarded-qualifiers -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-parentheses 
CFLAGS   := -mcpu=e907f   $(PreprocessorSwitch)CONFIG_CHIP_SUN20IW5  $(PreprocessorSwitch)DEBUG_MODE   -O0  -g3  -Wall  -ffunction-sections  -fdata-sections -nostdlib -nostdinc -march=rv32imafcxthead -mabi=ilp32f -Wno-int-to-pointer-cast -Wno-int-to-pointer-cast -Wno-shift-count-overflow -Wno-builtin-declaration-mismatch -Wno-pointer-to-int-cast -Wno-implicit-function-declaration -Wno-discarded-qualifiers -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-parentheses 
ASFLAGS  := -mcpu=e907f      
PreprocessFlags  := -mcpu=e907f   $(PreprocessorSwitch)CONFIG_CHIP_SUN20IW5  $(PreprocessorSwitch)DEBUG_MODE   -O0     -Wall  -ffunction-sections  -fdata-sections -nostdlib -nostdinc -march=rv32imafcxthead -mabi=ilp32f -Wno-int-to-pointer-cast -Wno-int-to-pointer-cast -Wno-shift-count-overflow -Wno-builtin-declaration-mismatch -Wno-pointer-to-int-cast -Wno-implicit-function-declaration -Wno-discarded-qualifiers -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-parentheses 


Objects0=$(IntermediateDirectory)/avaota-cam_board$(ObjectSuffix) $(IntermediateDirectory)/avaota-cam_eabi_compat$(ObjectSuffix) $(IntermediateDirectory)/avaota-cam_head$(ObjectSuffix) $(IntermediateDirectory)/avaota-cam_start$(ObjectSuffix) $(IntermediateDirectory)/src_common$(ObjectSuffix) $(IntermediateDirectory)/src_ctype$(ObjectSuffix) $(IntermediateDirectory)/src_fdt_wrapper$(ObjectSuffix) $(IntermediateDirectory)/src_os$(ObjectSuffix) $(IntermediateDirectory)/src_smalloc$(ObjectSuffix) $(IntermediateDirectory)/src_sstdlib$(ObjectSuffix) \
	$(IntermediateDirectory)/src_string$(ObjectSuffix) $(IntermediateDirectory)/src_uart$(ObjectSuffix) $(IntermediateDirectory)/app_main$(ObjectSuffix) $(IntermediateDirectory)/cli_commands$(ObjectSuffix) $(IntermediateDirectory)/cli_history$(ObjectSuffix) $(IntermediateDirectory)/cli_lineedit$(ObjectSuffix) $(IntermediateDirectory)/cli_parse$(ObjectSuffix) $(IntermediateDirectory)/cli_shell$(ObjectSuffix) $(IntermediateDirectory)/drivers_sys-clk$(ObjectSuffix) $(IntermediateDirectory)/drivers_sys-gpio$(ObjectSuffix) \
	$(IntermediateDirectory)/drivers_sys-uart$(ObjectSuffix) $(IntermediateDirectory)/drivers_sys-dma$(ObjectSuffix) $(IntermediateDirectory)/drivers_sys-dram$(ObjectSuffix) $(IntermediateDirectory)/drivers_sys-i2c$(ObjectSuffix) $(IntermediateDirectory)/drivers_sys-rtc$(ObjectSuffix) $(IntermediateDirectory)/drivers_sys-spi$(ObjectSuffix) $(IntermediateDirectory)/log_log$(ObjectSuffix) $(IntermediateDirectory)/log_xformat$(ObjectSuffix) $(IntermediateDirectory)/image_bimage$(ObjectSuffix) $(IntermediateDirectory)/image_uimage$(ObjectSuffix) \
	$(IntermediateDirectory)/image_zimage$(ObjectSuffix) 

Objects1=$(IntermediateDirectory)/riscv32_e907_exception$(ObjectSuffix) $(IntermediateDirectory)/riscv32_e907_fprw$(ObjectSuffix) $(IntermediateDirectory)/riscv32_e907_memcmp$(ObjectSuffix) $(IntermediateDirectory)/riscv32_e907_memcpy$(ObjectSuffix) $(IntermediateDirectory)/riscv32_e907_memset$(ObjectSuffix) $(IntermediateDirectory)/riscv32_e907_timer$(ObjectSuffix) $(IntermediateDirectory)/riscv32_e907_backtrace$(ObjectSuffix) $(IntermediateDirectory)/sun20iw5_sys-clk$(ObjectSuffix) $(IntermediateDirectory)/sun20iw5_sys-dram$(ObjectSuffix) \
	$(IntermediateDirectory)/sun20iw5_sys-rproc$(ObjectSuffix) $(IntermediateDirectory)/sun20iw5_sys-sid$(ObjectSuffix) $(IntermediateDirectory)/sun20iw5_sys-wdt$(ObjectSuffix) $(IntermediateDirectory)/sun20iw5_sys-sdhci$(ObjectSuffix) $(IntermediateDirectory)/mmc_sys-mmc$(ObjectSuffix) $(IntermediateDirectory)/mmc_sys-sdcard$(ObjectSuffix) $(IntermediateDirectory)/mmc_sys-sdhci$(ObjectSuffix) $(IntermediateDirectory)/pmu_axp$(ObjectSuffix) $(IntermediateDirectory)/pmu_axp1530$(ObjectSuffix) $(IntermediateDirectory)/pmu_axp2101$(ObjectSuffix) \
	$(IntermediateDirectory)/pmu_axp2202$(ObjectSuffix) $(IntermediateDirectory)/mtd_sys-spi-nand$(ObjectSuffix) $(IntermediateDirectory)/mtd_sys-spi-nor$(ObjectSuffix) 



Objects=$(Objects0) $(Objects1) 

##
## Main Build Targets 
##
.PHONY: all
all: $(IntermediateDirectory)/$(OutputFile)

$(IntermediateDirectory)/$(OutputFile):  $(Objects) Always_Link 
	$(LinkerName) $(OutputSwitch) $(IntermediateDirectory)/$(OutputFile)$(ExeSuffix) $(LinkerNameoption) -Wl,-Map=$(ProjectPath)/Lst/$(OutputFile).map  @$(ObjectsFileList)  $(LinkOptions) $(LibPath) $(Libs) $(LinkOtherFlagsOption)
	-@mv $(ProjectPath)/Lst/$(OutputFile).map $(ProjectPath)/Lst/$(OutputFile).temp && $(READELF) $(ElfInfoSwitch) $(ProjectPath)/Obj/$(OutputFile)$(ExeSuffix) > $(ProjectPath)/Lst/$(OutputFile).map && echo ====================================================================== >> $(ProjectPath)/Lst/$(OutputFile).map && cat $(ProjectPath)/Lst/$(OutputFile).temp >> $(ProjectPath)/Lst/$(OutputFile).map && rm -rf $(ProjectPath)/Lst/$(OutputFile).temp
	$(OBJCOPY) $(ObjcopySwitch) $(ProjectPath)/$(IntermediateDirectory)/$(OutputFile)$(ExeSuffix)  $(ProjectPath)/Obj/$(OutputFile)$(IHexSuffix) 
	$(OBJDUMP) $(ObjdumpSwitch) $(ProjectPath)/$(IntermediateDirectory)/$(OutputFile)$(ExeSuffix)  > $(ProjectPath)/Lst/$(OutputFile)$(DisassemSuffix) 
	@echo size of target:
	@$(SIZE) $(ProjectPath)$(IntermediateDirectory)/$(OutputFile)$(ExeSuffix) 
	@echo -n checksum value of target:  
	@$(CHECKSUM) $(ProjectPath)/$(IntermediateDirectory)/$(OutputFile)$(ExeSuffix) 
	@avaota_cam.modify.bat $(IntermediateDirectory) $(OutputFile)$(ExeSuffix) 

Always_Link:


##
## Objects
##
$(IntermediateDirectory)/avaota-cam_board$(ObjectSuffix): ../board.c  Lst/avaota-cam_board$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../board.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/avaota-cam_board$(ObjectSuffix) -MF$(IntermediateDirectory)/avaota-cam_board$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/avaota-cam_board$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/avaota-cam_board$(PreprocessSuffix): ../board.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/avaota-cam_board$(PreprocessSuffix) ../board.c

$(IntermediateDirectory)/avaota-cam_eabi_compat$(ObjectSuffix): ../eabi_compat.c  Lst/avaota-cam_eabi_compat$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../eabi_compat.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/avaota-cam_eabi_compat$(ObjectSuffix) -MF$(IntermediateDirectory)/avaota-cam_eabi_compat$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/avaota-cam_eabi_compat$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/avaota-cam_eabi_compat$(PreprocessSuffix): ../eabi_compat.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/avaota-cam_eabi_compat$(PreprocessSuffix) ../eabi_compat.c

$(IntermediateDirectory)/avaota-cam_head$(ObjectSuffix): ../head.c  Lst/avaota-cam_head$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../head.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/avaota-cam_head$(ObjectSuffix) -MF$(IntermediateDirectory)/avaota-cam_head$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/avaota-cam_head$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/avaota-cam_head$(PreprocessSuffix): ../head.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/avaota-cam_head$(PreprocessSuffix) ../head.c

$(IntermediateDirectory)/avaota-cam_start$(ObjectSuffix): ../start.S  Lst/avaota-cam_start$(PreprocessSuffix)
	$(AS) $(SourceSwitch) ../start.S $(ASFLAGS) -MMD -MP -MT$(IntermediateDirectory)/avaota-cam_start$(ObjectSuffix) -MF$(IntermediateDirectory)/avaota-cam_start$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/avaota-cam_start$(ObjectSuffix) $(IncludeAPath) $(IncludePackagePath)
Lst/avaota-cam_start$(PreprocessSuffix): ../start.S
	$(CC) $(CFLAGS)$(IncludeAPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/avaota-cam_start$(PreprocessSuffix) ../start.S

$(IntermediateDirectory)/src_common$(ObjectSuffix): ../../../src/common.c  Lst/src_common$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/common.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/src_common$(ObjectSuffix) -MF$(IntermediateDirectory)/src_common$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/src_common$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/src_common$(PreprocessSuffix): ../../../src/common.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/src_common$(PreprocessSuffix) ../../../src/common.c

$(IntermediateDirectory)/src_ctype$(ObjectSuffix): ../../../src/ctype.c  Lst/src_ctype$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/ctype.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/src_ctype$(ObjectSuffix) -MF$(IntermediateDirectory)/src_ctype$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/src_ctype$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/src_ctype$(PreprocessSuffix): ../../../src/ctype.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/src_ctype$(PreprocessSuffix) ../../../src/ctype.c

$(IntermediateDirectory)/src_fdt_wrapper$(ObjectSuffix): ../../../src/fdt_wrapper.c  Lst/src_fdt_wrapper$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/fdt_wrapper.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/src_fdt_wrapper$(ObjectSuffix) -MF$(IntermediateDirectory)/src_fdt_wrapper$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/src_fdt_wrapper$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/src_fdt_wrapper$(PreprocessSuffix): ../../../src/fdt_wrapper.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/src_fdt_wrapper$(PreprocessSuffix) ../../../src/fdt_wrapper.c

$(IntermediateDirectory)/src_os$(ObjectSuffix): ../../../src/os.c  Lst/src_os$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/os.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/src_os$(ObjectSuffix) -MF$(IntermediateDirectory)/src_os$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/src_os$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/src_os$(PreprocessSuffix): ../../../src/os.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/src_os$(PreprocessSuffix) ../../../src/os.c

$(IntermediateDirectory)/src_smalloc$(ObjectSuffix): ../../../src/smalloc.c  Lst/src_smalloc$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/smalloc.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/src_smalloc$(ObjectSuffix) -MF$(IntermediateDirectory)/src_smalloc$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/src_smalloc$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/src_smalloc$(PreprocessSuffix): ../../../src/smalloc.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/src_smalloc$(PreprocessSuffix) ../../../src/smalloc.c

$(IntermediateDirectory)/src_sstdlib$(ObjectSuffix): ../../../src/sstdlib.c  Lst/src_sstdlib$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/sstdlib.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/src_sstdlib$(ObjectSuffix) -MF$(IntermediateDirectory)/src_sstdlib$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/src_sstdlib$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/src_sstdlib$(PreprocessSuffix): ../../../src/sstdlib.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/src_sstdlib$(PreprocessSuffix) ../../../src/sstdlib.c

$(IntermediateDirectory)/src_string$(ObjectSuffix): ../../../src/string.c  Lst/src_string$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/string.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/src_string$(ObjectSuffix) -MF$(IntermediateDirectory)/src_string$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/src_string$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/src_string$(PreprocessSuffix): ../../../src/string.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/src_string$(PreprocessSuffix) ../../../src/string.c

$(IntermediateDirectory)/src_uart$(ObjectSuffix): ../../../src/uart.c  Lst/src_uart$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/uart.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/src_uart$(ObjectSuffix) -MF$(IntermediateDirectory)/src_uart$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/src_uart$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/src_uart$(PreprocessSuffix): ../../../src/uart.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/src_uart$(PreprocessSuffix) ../../../src/uart.c

$(IntermediateDirectory)/app_main$(ObjectSuffix): ../app/main.c  Lst/app_main$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../app/main.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/app_main$(ObjectSuffix) -MF$(IntermediateDirectory)/app_main$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/app_main$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/app_main$(PreprocessSuffix): ../app/main.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/app_main$(PreprocessSuffix) ../app/main.c

$(IntermediateDirectory)/cli_commands$(ObjectSuffix): ../../../src/cli/commands.c  Lst/cli_commands$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/cli/commands.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/cli_commands$(ObjectSuffix) -MF$(IntermediateDirectory)/cli_commands$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/cli_commands$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/cli_commands$(PreprocessSuffix): ../../../src/cli/commands.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/cli_commands$(PreprocessSuffix) ../../../src/cli/commands.c

$(IntermediateDirectory)/cli_history$(ObjectSuffix): ../../../src/cli/history.c  Lst/cli_history$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/cli/history.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/cli_history$(ObjectSuffix) -MF$(IntermediateDirectory)/cli_history$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/cli_history$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/cli_history$(PreprocessSuffix): ../../../src/cli/history.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/cli_history$(PreprocessSuffix) ../../../src/cli/history.c

$(IntermediateDirectory)/cli_lineedit$(ObjectSuffix): ../../../src/cli/lineedit.c  Lst/cli_lineedit$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/cli/lineedit.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/cli_lineedit$(ObjectSuffix) -MF$(IntermediateDirectory)/cli_lineedit$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/cli_lineedit$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/cli_lineedit$(PreprocessSuffix): ../../../src/cli/lineedit.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/cli_lineedit$(PreprocessSuffix) ../../../src/cli/lineedit.c

$(IntermediateDirectory)/cli_parse$(ObjectSuffix): ../../../src/cli/parse.c  Lst/cli_parse$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/cli/parse.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/cli_parse$(ObjectSuffix) -MF$(IntermediateDirectory)/cli_parse$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/cli_parse$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/cli_parse$(PreprocessSuffix): ../../../src/cli/parse.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/cli_parse$(PreprocessSuffix) ../../../src/cli/parse.c

$(IntermediateDirectory)/cli_shell$(ObjectSuffix): ../../../src/cli/shell.c  Lst/cli_shell$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/cli/shell.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/cli_shell$(ObjectSuffix) -MF$(IntermediateDirectory)/cli_shell$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/cli_shell$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/cli_shell$(PreprocessSuffix): ../../../src/cli/shell.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/cli_shell$(PreprocessSuffix) ../../../src/cli/shell.c

$(IntermediateDirectory)/drivers_sys-clk$(ObjectSuffix): ../../../src/drivers/sys-clk.c  Lst/drivers_sys-clk$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/drivers/sys-clk.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/drivers_sys-clk$(ObjectSuffix) -MF$(IntermediateDirectory)/drivers_sys-clk$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/drivers_sys-clk$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/drivers_sys-clk$(PreprocessSuffix): ../../../src/drivers/sys-clk.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/drivers_sys-clk$(PreprocessSuffix) ../../../src/drivers/sys-clk.c

$(IntermediateDirectory)/drivers_sys-gpio$(ObjectSuffix): ../../../src/drivers/sys-gpio.c  Lst/drivers_sys-gpio$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/drivers/sys-gpio.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/drivers_sys-gpio$(ObjectSuffix) -MF$(IntermediateDirectory)/drivers_sys-gpio$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/drivers_sys-gpio$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/drivers_sys-gpio$(PreprocessSuffix): ../../../src/drivers/sys-gpio.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/drivers_sys-gpio$(PreprocessSuffix) ../../../src/drivers/sys-gpio.c

$(IntermediateDirectory)/drivers_sys-uart$(ObjectSuffix): ../../../src/drivers/sys-uart.c  Lst/drivers_sys-uart$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/drivers/sys-uart.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/drivers_sys-uart$(ObjectSuffix) -MF$(IntermediateDirectory)/drivers_sys-uart$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/drivers_sys-uart$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/drivers_sys-uart$(PreprocessSuffix): ../../../src/drivers/sys-uart.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/drivers_sys-uart$(PreprocessSuffix) ../../../src/drivers/sys-uart.c

$(IntermediateDirectory)/drivers_sys-dma$(ObjectSuffix): ../../../src/drivers/sys-dma.c  Lst/drivers_sys-dma$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/drivers/sys-dma.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/drivers_sys-dma$(ObjectSuffix) -MF$(IntermediateDirectory)/drivers_sys-dma$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/drivers_sys-dma$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/drivers_sys-dma$(PreprocessSuffix): ../../../src/drivers/sys-dma.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/drivers_sys-dma$(PreprocessSuffix) ../../../src/drivers/sys-dma.c

$(IntermediateDirectory)/drivers_sys-dram$(ObjectSuffix): ../../../src/drivers/sys-dram.c  Lst/drivers_sys-dram$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/drivers/sys-dram.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/drivers_sys-dram$(ObjectSuffix) -MF$(IntermediateDirectory)/drivers_sys-dram$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/drivers_sys-dram$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/drivers_sys-dram$(PreprocessSuffix): ../../../src/drivers/sys-dram.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/drivers_sys-dram$(PreprocessSuffix) ../../../src/drivers/sys-dram.c

$(IntermediateDirectory)/drivers_sys-i2c$(ObjectSuffix): ../../../src/drivers/sys-i2c.c  Lst/drivers_sys-i2c$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/drivers/sys-i2c.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/drivers_sys-i2c$(ObjectSuffix) -MF$(IntermediateDirectory)/drivers_sys-i2c$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/drivers_sys-i2c$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/drivers_sys-i2c$(PreprocessSuffix): ../../../src/drivers/sys-i2c.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/drivers_sys-i2c$(PreprocessSuffix) ../../../src/drivers/sys-i2c.c

$(IntermediateDirectory)/drivers_sys-rtc$(ObjectSuffix): ../../../src/drivers/sys-rtc.c  Lst/drivers_sys-rtc$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/drivers/sys-rtc.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/drivers_sys-rtc$(ObjectSuffix) -MF$(IntermediateDirectory)/drivers_sys-rtc$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/drivers_sys-rtc$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/drivers_sys-rtc$(PreprocessSuffix): ../../../src/drivers/sys-rtc.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/drivers_sys-rtc$(PreprocessSuffix) ../../../src/drivers/sys-rtc.c

$(IntermediateDirectory)/drivers_sys-spi$(ObjectSuffix): ../../../src/drivers/sys-spi.c  Lst/drivers_sys-spi$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/drivers/sys-spi.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/drivers_sys-spi$(ObjectSuffix) -MF$(IntermediateDirectory)/drivers_sys-spi$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/drivers_sys-spi$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/drivers_sys-spi$(PreprocessSuffix): ../../../src/drivers/sys-spi.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/drivers_sys-spi$(PreprocessSuffix) ../../../src/drivers/sys-spi.c

$(IntermediateDirectory)/log_log$(ObjectSuffix): ../../../src/log/log.c  Lst/log_log$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/log/log.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/log_log$(ObjectSuffix) -MF$(IntermediateDirectory)/log_log$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/log_log$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/log_log$(PreprocessSuffix): ../../../src/log/log.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/log_log$(PreprocessSuffix) ../../../src/log/log.c

$(IntermediateDirectory)/log_xformat$(ObjectSuffix): ../../../src/log/xformat.c  Lst/log_xformat$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/log/xformat.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/log_xformat$(ObjectSuffix) -MF$(IntermediateDirectory)/log_xformat$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/log_xformat$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/log_xformat$(PreprocessSuffix): ../../../src/log/xformat.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/log_xformat$(PreprocessSuffix) ../../../src/log/xformat.c

$(IntermediateDirectory)/image_bimage$(ObjectSuffix): ../../../src/image/bimage.c  Lst/image_bimage$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/image/bimage.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/image_bimage$(ObjectSuffix) -MF$(IntermediateDirectory)/image_bimage$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/image_bimage$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/image_bimage$(PreprocessSuffix): ../../../src/image/bimage.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/image_bimage$(PreprocessSuffix) ../../../src/image/bimage.c

$(IntermediateDirectory)/image_uimage$(ObjectSuffix): ../../../src/image/uimage.c  Lst/image_uimage$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/image/uimage.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/image_uimage$(ObjectSuffix) -MF$(IntermediateDirectory)/image_uimage$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/image_uimage$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/image_uimage$(PreprocessSuffix): ../../../src/image/uimage.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/image_uimage$(PreprocessSuffix) ../../../src/image/uimage.c

$(IntermediateDirectory)/image_zimage$(ObjectSuffix): ../../../src/image/zimage.c  Lst/image_zimage$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/image/zimage.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/image_zimage$(ObjectSuffix) -MF$(IntermediateDirectory)/image_zimage$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/image_zimage$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/image_zimage$(PreprocessSuffix): ../../../src/image/zimage.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/image_zimage$(PreprocessSuffix) ../../../src/image/zimage.c

$(IntermediateDirectory)/riscv32_e907_exception$(ObjectSuffix): ../../../src/arch/riscv/riscv32_e907/exception.c  Lst/riscv32_e907_exception$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/arch/riscv/riscv32_e907/exception.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/riscv32_e907_exception$(ObjectSuffix) -MF$(IntermediateDirectory)/riscv32_e907_exception$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/riscv32_e907_exception$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/riscv32_e907_exception$(PreprocessSuffix): ../../../src/arch/riscv/riscv32_e907/exception.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/riscv32_e907_exception$(PreprocessSuffix) ../../../src/arch/riscv/riscv32_e907/exception.c

$(IntermediateDirectory)/riscv32_e907_fprw$(ObjectSuffix): ../../../src/arch/riscv/riscv32_e907/fprw.S  Lst/riscv32_e907_fprw$(PreprocessSuffix)
	$(AS) $(SourceSwitch) ../../../src/arch/riscv/riscv32_e907/fprw.S $(ASFLAGS) -MMD -MP -MT$(IntermediateDirectory)/riscv32_e907_fprw$(ObjectSuffix) -MF$(IntermediateDirectory)/riscv32_e907_fprw$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/riscv32_e907_fprw$(ObjectSuffix) $(IncludeAPath) $(IncludePackagePath)
Lst/riscv32_e907_fprw$(PreprocessSuffix): ../../../src/arch/riscv/riscv32_e907/fprw.S
	$(CC) $(CFLAGS)$(IncludeAPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/riscv32_e907_fprw$(PreprocessSuffix) ../../../src/arch/riscv/riscv32_e907/fprw.S

$(IntermediateDirectory)/riscv32_e907_memcmp$(ObjectSuffix): ../../../src/arch/riscv/riscv32_e907/memcmp.c  Lst/riscv32_e907_memcmp$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/arch/riscv/riscv32_e907/memcmp.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/riscv32_e907_memcmp$(ObjectSuffix) -MF$(IntermediateDirectory)/riscv32_e907_memcmp$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/riscv32_e907_memcmp$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/riscv32_e907_memcmp$(PreprocessSuffix): ../../../src/arch/riscv/riscv32_e907/memcmp.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/riscv32_e907_memcmp$(PreprocessSuffix) ../../../src/arch/riscv/riscv32_e907/memcmp.c

$(IntermediateDirectory)/riscv32_e907_memcpy$(ObjectSuffix): ../../../src/arch/riscv/riscv32_e907/memcpy.S  Lst/riscv32_e907_memcpy$(PreprocessSuffix)
	$(AS) $(SourceSwitch) ../../../src/arch/riscv/riscv32_e907/memcpy.S $(ASFLAGS) -MMD -MP -MT$(IntermediateDirectory)/riscv32_e907_memcpy$(ObjectSuffix) -MF$(IntermediateDirectory)/riscv32_e907_memcpy$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/riscv32_e907_memcpy$(ObjectSuffix) $(IncludeAPath) $(IncludePackagePath)
Lst/riscv32_e907_memcpy$(PreprocessSuffix): ../../../src/arch/riscv/riscv32_e907/memcpy.S
	$(CC) $(CFLAGS)$(IncludeAPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/riscv32_e907_memcpy$(PreprocessSuffix) ../../../src/arch/riscv/riscv32_e907/memcpy.S

$(IntermediateDirectory)/riscv32_e907_memset$(ObjectSuffix): ../../../src/arch/riscv/riscv32_e907/memset.S  Lst/riscv32_e907_memset$(PreprocessSuffix)
	$(AS) $(SourceSwitch) ../../../src/arch/riscv/riscv32_e907/memset.S $(ASFLAGS) -MMD -MP -MT$(IntermediateDirectory)/riscv32_e907_memset$(ObjectSuffix) -MF$(IntermediateDirectory)/riscv32_e907_memset$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/riscv32_e907_memset$(ObjectSuffix) $(IncludeAPath) $(IncludePackagePath)
Lst/riscv32_e907_memset$(PreprocessSuffix): ../../../src/arch/riscv/riscv32_e907/memset.S
	$(CC) $(CFLAGS)$(IncludeAPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/riscv32_e907_memset$(PreprocessSuffix) ../../../src/arch/riscv/riscv32_e907/memset.S

$(IntermediateDirectory)/riscv32_e907_timer$(ObjectSuffix): ../../../src/arch/riscv/riscv32_e907/timer.c  Lst/riscv32_e907_timer$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/arch/riscv/riscv32_e907/timer.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/riscv32_e907_timer$(ObjectSuffix) -MF$(IntermediateDirectory)/riscv32_e907_timer$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/riscv32_e907_timer$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/riscv32_e907_timer$(PreprocessSuffix): ../../../src/arch/riscv/riscv32_e907/timer.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/riscv32_e907_timer$(PreprocessSuffix) ../../../src/arch/riscv/riscv32_e907/timer.c

$(IntermediateDirectory)/riscv32_e907_backtrace$(ObjectSuffix): ../../../src/arch/riscv/riscv32_e907/backtrace.c  Lst/riscv32_e907_backtrace$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/arch/riscv/riscv32_e907/backtrace.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/riscv32_e907_backtrace$(ObjectSuffix) -MF$(IntermediateDirectory)/riscv32_e907_backtrace$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/riscv32_e907_backtrace$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/riscv32_e907_backtrace$(PreprocessSuffix): ../../../src/arch/riscv/riscv32_e907/backtrace.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/riscv32_e907_backtrace$(PreprocessSuffix) ../../../src/arch/riscv/riscv32_e907/backtrace.c

$(IntermediateDirectory)/sun20iw5_sys-clk$(ObjectSuffix): ../../../src/drivers/chips/sun20iw5/sys-clk.c  Lst/sun20iw5_sys-clk$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/drivers/chips/sun20iw5/sys-clk.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/sun20iw5_sys-clk$(ObjectSuffix) -MF$(IntermediateDirectory)/sun20iw5_sys-clk$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/sun20iw5_sys-clk$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/sun20iw5_sys-clk$(PreprocessSuffix): ../../../src/drivers/chips/sun20iw5/sys-clk.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/sun20iw5_sys-clk$(PreprocessSuffix) ../../../src/drivers/chips/sun20iw5/sys-clk.c

$(IntermediateDirectory)/sun20iw5_sys-dram$(ObjectSuffix): ../../../src/drivers/chips/sun20iw5/sys-dram.c  Lst/sun20iw5_sys-dram$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/drivers/chips/sun20iw5/sys-dram.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/sun20iw5_sys-dram$(ObjectSuffix) -MF$(IntermediateDirectory)/sun20iw5_sys-dram$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/sun20iw5_sys-dram$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/sun20iw5_sys-dram$(PreprocessSuffix): ../../../src/drivers/chips/sun20iw5/sys-dram.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/sun20iw5_sys-dram$(PreprocessSuffix) ../../../src/drivers/chips/sun20iw5/sys-dram.c

$(IntermediateDirectory)/sun20iw5_sys-rproc$(ObjectSuffix): ../../../src/drivers/chips/sun20iw5/sys-rproc.c  Lst/sun20iw5_sys-rproc$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/drivers/chips/sun20iw5/sys-rproc.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/sun20iw5_sys-rproc$(ObjectSuffix) -MF$(IntermediateDirectory)/sun20iw5_sys-rproc$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/sun20iw5_sys-rproc$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/sun20iw5_sys-rproc$(PreprocessSuffix): ../../../src/drivers/chips/sun20iw5/sys-rproc.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/sun20iw5_sys-rproc$(PreprocessSuffix) ../../../src/drivers/chips/sun20iw5/sys-rproc.c

$(IntermediateDirectory)/sun20iw5_sys-sid$(ObjectSuffix): ../../../src/drivers/chips/sun20iw5/sys-sid.c  Lst/sun20iw5_sys-sid$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/drivers/chips/sun20iw5/sys-sid.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/sun20iw5_sys-sid$(ObjectSuffix) -MF$(IntermediateDirectory)/sun20iw5_sys-sid$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/sun20iw5_sys-sid$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/sun20iw5_sys-sid$(PreprocessSuffix): ../../../src/drivers/chips/sun20iw5/sys-sid.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/sun20iw5_sys-sid$(PreprocessSuffix) ../../../src/drivers/chips/sun20iw5/sys-sid.c

$(IntermediateDirectory)/sun20iw5_sys-wdt$(ObjectSuffix): ../../../src/drivers/chips/sun20iw5/sys-wdt.c  Lst/sun20iw5_sys-wdt$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/drivers/chips/sun20iw5/sys-wdt.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/sun20iw5_sys-wdt$(ObjectSuffix) -MF$(IntermediateDirectory)/sun20iw5_sys-wdt$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/sun20iw5_sys-wdt$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/sun20iw5_sys-wdt$(PreprocessSuffix): ../../../src/drivers/chips/sun20iw5/sys-wdt.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/sun20iw5_sys-wdt$(PreprocessSuffix) ../../../src/drivers/chips/sun20iw5/sys-wdt.c

$(IntermediateDirectory)/sun20iw5_sys-sdhci$(ObjectSuffix): ../../../src/drivers/chips/sun20iw5/sys-sdhci.c  Lst/sun20iw5_sys-sdhci$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/drivers/chips/sun20iw5/sys-sdhci.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/sun20iw5_sys-sdhci$(ObjectSuffix) -MF$(IntermediateDirectory)/sun20iw5_sys-sdhci$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/sun20iw5_sys-sdhci$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/sun20iw5_sys-sdhci$(PreprocessSuffix): ../../../src/drivers/chips/sun20iw5/sys-sdhci.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/sun20iw5_sys-sdhci$(PreprocessSuffix) ../../../src/drivers/chips/sun20iw5/sys-sdhci.c

$(IntermediateDirectory)/mmc_sys-mmc$(ObjectSuffix): ../../../src/drivers/mmc/sys-mmc.c  Lst/mmc_sys-mmc$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/drivers/mmc/sys-mmc.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/mmc_sys-mmc$(ObjectSuffix) -MF$(IntermediateDirectory)/mmc_sys-mmc$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/mmc_sys-mmc$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/mmc_sys-mmc$(PreprocessSuffix): ../../../src/drivers/mmc/sys-mmc.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/mmc_sys-mmc$(PreprocessSuffix) ../../../src/drivers/mmc/sys-mmc.c

$(IntermediateDirectory)/mmc_sys-sdcard$(ObjectSuffix): ../../../src/drivers/mmc/sys-sdcard.c  Lst/mmc_sys-sdcard$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/drivers/mmc/sys-sdcard.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/mmc_sys-sdcard$(ObjectSuffix) -MF$(IntermediateDirectory)/mmc_sys-sdcard$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/mmc_sys-sdcard$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/mmc_sys-sdcard$(PreprocessSuffix): ../../../src/drivers/mmc/sys-sdcard.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/mmc_sys-sdcard$(PreprocessSuffix) ../../../src/drivers/mmc/sys-sdcard.c

$(IntermediateDirectory)/mmc_sys-sdhci$(ObjectSuffix): ../../../src/drivers/mmc/sys-sdhci.c  Lst/mmc_sys-sdhci$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/drivers/mmc/sys-sdhci.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/mmc_sys-sdhci$(ObjectSuffix) -MF$(IntermediateDirectory)/mmc_sys-sdhci$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/mmc_sys-sdhci$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/mmc_sys-sdhci$(PreprocessSuffix): ../../../src/drivers/mmc/sys-sdhci.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/mmc_sys-sdhci$(PreprocessSuffix) ../../../src/drivers/mmc/sys-sdhci.c

$(IntermediateDirectory)/pmu_axp$(ObjectSuffix): ../../../src/drivers/pmu/axp.c  Lst/pmu_axp$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/drivers/pmu/axp.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/pmu_axp$(ObjectSuffix) -MF$(IntermediateDirectory)/pmu_axp$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/pmu_axp$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/pmu_axp$(PreprocessSuffix): ../../../src/drivers/pmu/axp.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/pmu_axp$(PreprocessSuffix) ../../../src/drivers/pmu/axp.c

$(IntermediateDirectory)/pmu_axp1530$(ObjectSuffix): ../../../src/drivers/pmu/axp1530.c  Lst/pmu_axp1530$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/drivers/pmu/axp1530.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/pmu_axp1530$(ObjectSuffix) -MF$(IntermediateDirectory)/pmu_axp1530$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/pmu_axp1530$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/pmu_axp1530$(PreprocessSuffix): ../../../src/drivers/pmu/axp1530.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/pmu_axp1530$(PreprocessSuffix) ../../../src/drivers/pmu/axp1530.c

$(IntermediateDirectory)/pmu_axp2101$(ObjectSuffix): ../../../src/drivers/pmu/axp2101.c  Lst/pmu_axp2101$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/drivers/pmu/axp2101.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/pmu_axp2101$(ObjectSuffix) -MF$(IntermediateDirectory)/pmu_axp2101$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/pmu_axp2101$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/pmu_axp2101$(PreprocessSuffix): ../../../src/drivers/pmu/axp2101.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/pmu_axp2101$(PreprocessSuffix) ../../../src/drivers/pmu/axp2101.c

$(IntermediateDirectory)/pmu_axp2202$(ObjectSuffix): ../../../src/drivers/pmu/axp2202.c  Lst/pmu_axp2202$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/drivers/pmu/axp2202.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/pmu_axp2202$(ObjectSuffix) -MF$(IntermediateDirectory)/pmu_axp2202$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/pmu_axp2202$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/pmu_axp2202$(PreprocessSuffix): ../../../src/drivers/pmu/axp2202.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/pmu_axp2202$(PreprocessSuffix) ../../../src/drivers/pmu/axp2202.c

$(IntermediateDirectory)/mtd_sys-spi-nand$(ObjectSuffix): ../../../src/drivers/mtd/sys-spi-nand.c  Lst/mtd_sys-spi-nand$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/drivers/mtd/sys-spi-nand.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/mtd_sys-spi-nand$(ObjectSuffix) -MF$(IntermediateDirectory)/mtd_sys-spi-nand$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/mtd_sys-spi-nand$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/mtd_sys-spi-nand$(PreprocessSuffix): ../../../src/drivers/mtd/sys-spi-nand.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/mtd_sys-spi-nand$(PreprocessSuffix) ../../../src/drivers/mtd/sys-spi-nand.c

$(IntermediateDirectory)/mtd_sys-spi-nor$(ObjectSuffix): ../../../src/drivers/mtd/sys-spi-nor.c  Lst/mtd_sys-spi-nor$(PreprocessSuffix)
	$(CC) $(SourceSwitch) ../../../src/drivers/mtd/sys-spi-nor.c $(CFLAGS) -MMD -MP -MT$(IntermediateDirectory)/mtd_sys-spi-nor$(ObjectSuffix) -MF$(IntermediateDirectory)/mtd_sys-spi-nor$(DependSuffix) $(ObjectSwitch)$(IntermediateDirectory)/mtd_sys-spi-nor$(ObjectSuffix) $(IncludeCPath) $(IncludePackagePath)
Lst/mtd_sys-spi-nor$(PreprocessSuffix): ../../../src/drivers/mtd/sys-spi-nor.c
	$(CC) $(CFLAGS)$(IncludeCPath) $(PreprocessOnlySwitch) $(OutputSwitch) Lst/mtd_sys-spi-nor$(PreprocessSuffix) ../../../src/drivers/mtd/sys-spi-nor.c


-include $(IntermediateDirectory)/*$(DependSuffix)
