# SPDX-License-Identifier: GPL-2.0+

src := $(srctree)/boards/$(board)/$(app)
out := $(objtree)/.obj/apps/$(board)/$(app)

include $(objtree)/include/config/auto.conf
include $(srctree)/boards/$(board)/Makefile

local-objects := main.o $($(app)-y)
objects := $(addprefix $(out)/,$(local-objects))
deps := $(objects:.o=.d)
target := $(out)/built-in.o

.PHONY: all
all: $(target)

$(target): $(objects)
	@mkdir -p $(dir $@)
	@$(RM) $@
	@$(LD) $(KBUILD_LDFLAGS) -r -o $@ $^

$(out)/%.o: $(src)/%.c $(objtree)/include/generated/autoconf.h
	@mkdir -p $(dir $@)
	@echo "  CC      $(patsubst $(srctree)/%,%,$<)"
	@$(CC) $(KBUILD_CPPFLAGS) $(KBUILD_CFLAGS) $(ccflags-y) \
		$(CFLAGS_$(@F)) -MMD -MP -c $< -o $@

$(out)/%.o: $(src)/%.S $(objtree)/include/generated/autoconf.h
	@mkdir -p $(dir $@)
	@echo "  AS      $(patsubst $(srctree)/%,%,$<)"
	@$(CC) $(KBUILD_CPPFLAGS) $(KBUILD_AFLAGS) $(asflags-y) \
		$(AFLAGS_$(@F)) -MMD -MP -c $< -o $@

-include $(deps)
