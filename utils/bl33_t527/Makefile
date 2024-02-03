#
# Top makefile
#

CROSS		?= arm-none-eabi-
NAME		:= syter_bl33

#
# System environment variable.
#
ifeq ($(OS), Windows_NT)
	HOSTOS		:= windows
else
	ifneq (,$(findstring Linux, $(shell uname -a)))
		HOSTOS	:= linux
	endif
endif

#
# Load default variables.
#
ASFLAGS		:= -g -ggdb -Wall -O3
CFLAGS		:= -g -ggdb -Wall -O3
CXXFLAGS	:= -g -ggdb -Wall -O3
LDFLAGS		:= -T link.ld -nostdlib
ARFLAGS		:= -rcs
OCFLAGS		:= -v -O binary
ODFLAGS		:=
MCFLAGS		:= -nostdinc -nostdlib -mabi=aapcs-linux -march=armv7-a -Wno-builtin-declaration-mismatch -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast

LIBDIRS		:= 
LIBS 		:= 
INCDIRS		:=
SRCDIRS		:=

#
# Add external library
#
INCDIRS		+= include
SRCDIRS		+= source

#
# You shouldn't need to change anything below this point.
#
AS			:= $(CROSS)gcc -x assembler-with-cpp
CC			:= $(CROSS)gcc
CXX			:= $(CROSS)g++
LD			:= $(CROSS)ld
AR			:= $(CROSS)ar
OC			:= $(CROSS)objcopy
OD			:= $(CROSS)objdump
MKDIR		:= mkdir -p
CP			:= cp -af
RM			:= rm -fr
CD			:= cd
FIND		:= find

#
# X variables
#
G_ASFLAGS	:= $(MCFLAGS) $(ASFLAGS)
G_CFLAGS	:= $(MCFLAGS) $(CFLAGS)
G_CXXFLAGS	:= $(MCFLAGS) $(CXXFLAGS)
G_LDFLAGS	:= $(LDFLAGS)
G_OCFLAGS	:= $(OCFLAGS)
G_LIBDIRS	:= $(LIBDIRS)
G_LIBS		:= $(LIBS) -lgcc

G_OUT		:= output
G_NAME		:= $(patsubst %, $(G_OUT)/%, $(NAME))
G_INCDIRS	:= $(patsubst %, -I %, $(INCDIRS))
G_SRCDIRS	:= $(patsubst %, %, $(SRCDIRS))
G_OBJDIRS	:= $(patsubst %, .obj/%, $(G_SRCDIRS))

G_SFILES	:= $(foreach dir, $(G_SRCDIRS), $(wildcard $(dir)/*.S))
G_CFILES	:= $(foreach dir, $(G_SRCDIRS), $(wildcard $(dir)/*.c))
G_CPPFILES	:= $(foreach dir, $(G_SRCDIRS), $(wildcard $(dir)/*.cpp))

G_SDEPS		:= $(patsubst %, .obj/%, $(G_SFILES:.S=.o.d))
G_CDEPS		:= $(patsubst %, .obj/%, $(G_CFILES:.c=.o.d))
G_CPPDEPS	:= $(patsubst %, .obj/%, $(G_CPPFILES:.cpp=.o.d))
G_DEPS		:= $(G_SDEPS) $(G_CDEPS) $(G_CPPDEPS)

G_SOBJS		:= $(patsubst %, .obj/%, $(G_SFILES:.S=.o))
G_COBJS		:= $(patsubst %, .obj/%, $(G_CFILES:.c=.o))
G_CPPOBJS	:= $(patsubst %, .obj/%, $(G_CPPFILES:.cpp=.o)) 
G_OBJS		:= $(G_SOBJS) $(G_COBJS) $(G_CPPOBJS)

VPATH		:= $(G_OBJDIRS)

.PHONY:	all clean
all : $(G_NAME)

$(G_NAME) : $(G_OBJS)
	@echo [LD] Linking $@.elf
	@$(CC) $(G_LDFLAGS) $(G_LIBDIRS) -Wl,--cref,-Map=$@.map $^ -o $@.elf $(G_LIBS)
	@echo [OC] Objcopying $@.bin
	@$(OC) $(G_OCFLAGS) $@.elf $@.bin

$(G_SOBJS) : .obj/%.o : %.S
	@echo [AS] $<
	@$(AS) $(G_ASFLAGS) -MD -MP -MF $@.d $(G_INCDIRS) -c $< -o $@

$(G_COBJS) : .obj/%.o : %.c
	@echo [CC] $<
	@$(CC) $(G_CFLAGS) -MD -MP -MF $@.d $(G_INCDIRS) -c $< -o $@

$(G_CPPOBJS) : .obj/%.o : %.cpp
	@echo [CXX] $<
	@$(CXX) $(G_CXXFLAGS) -MD -MP -MF $@.d $(G_INCDIRS) -c $< -o $@

clean:
	@$(RM) .obj $(G_OUT)

#
# Include the dependency files, should be place the last of makefile
#
sinclude $(shell $(MKDIR) $(G_OBJDIRS) $(G_OUT)) $(G_DEPS)
