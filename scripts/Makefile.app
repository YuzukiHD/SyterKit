# SPDX-License-Identifier: GPL-2.0+

RM ?= rm -f

src := $(srctree)/$(platform_dir)/$(mode)/$(app)
out := $(objtree)/.obj/apps/$(target_name)/$(mode)/$(app)

include $(objtree)/include/config/auto.conf
include $(srctree)/$(platform_dir)/Makefile

# The platform metadata supplies both the app object list and local flags.
# Track it (and this worker) so incremental builds reflect those changes.
build_inputs += $(srctree)/scripts/Makefile.app

local-objects := main.o $($(app)-y)
objects := $(addprefix $(out)/,$(local-objects))
deps := $(objects:.o=.d)
target := $(out)/built-in.o

.PHONY: all
all: $(target)

$(target): $(objects) $(build_inputs)
	@mkdir -p $(dir $@)
	@$(RM) $@
	@$(LD) $(KBUILD_LDFLAGS) -r -o $@ $(objects)

$(out)/%.o: $(src)/%.c $(objtree)/include/generated/autoconf.h $(build_inputs)
	@mkdir -p $(dir $@)
	@echo "  CC      $(patsubst $(srctree)/%,%,$<)"
	@$(CC) $(KBUILD_CPPFLAGS) $(KBUILD_CFLAGS) $(ccflags-y) \
		$(CFLAGS_$(@F)) -MMD -MP -c $< -o $@

$(out)/%.o: $(src)/%.S $(objtree)/include/generated/autoconf.h $(build_inputs)
	@mkdir -p $(dir $@)
	@echo "  AS      $(patsubst $(srctree)/%,%,$<)"
	@$(CC) $(KBUILD_CPPFLAGS) $(KBUILD_AFLAGS) $(asflags-y) \
		$(AFLAGS_$(@F)) -MMD -MP -c $< -o $@

-include $(deps)
