# SPDX-License-Identifier: GPL-2.0

src := $(CURDIR)
O ?= $(src)
obj := $(abspath $(O))
quiet_path = scripts/kconfig/$(patsubst $(obj)/%,%,$(patsubst $(src)/%,%,$(1)))

HOSTCC ?= cc
HOSTPKG_CONFIG ?= pkg-config
HOSTCFLAGS ?= -O2 -g
HOSTCFLAGS += -D_GNU_SOURCE -DYYDEBUG -I$(src) -I$(obj)

common-objs := confdata.o expr.o lexer.lex.o menu.o parser.tab.o \
	preprocess.o symbol.o util.o
conf-objs := conf.o $(common-objs)
mconf-objs := mconf.o $(addprefix lxdialog/,checklist.o inputbox.o \
	menubox.o textbox.o util.o yesno.o) $(common-objs)

.PHONY: all clean
all: $(obj)/conf $(obj)/mconf

$(obj):
	@mkdir -p $@

$(obj)/lxdialog:
	@mkdir -p $@

$(obj)/parser.tab.c $(obj)/parser.tab.h &: $(src)/parser.y | $(obj)
	@echo "  BISON   parser.y"
	@bison -o $(obj)/parser.tab.c --defines=$(obj)/parser.tab.h $<

$(obj)/lexer.lex.c: $(src)/lexer.l $(obj)/parser.tab.h | $(obj)
	@echo "  FLEX    lexer.l"
	@flex -o $@ $<

$(obj)/mconf-cflags $(obj)/mconf-libs &: $(src)/mconf-cfg.sh | $(obj)
	@HOSTCC="$(HOSTCC)" HOSTPKG_CONFIG="$(HOSTPKG_CONFIG)" \
		$(CONFIG_SHELL) $< $(obj)/mconf-cflags $(obj)/mconf-libs

$(obj)/%.o: $(src)/%.c | $(obj)
	@mkdir -p $(dir $@)
	@echo "  HOSTCC  $(call quiet_path,$<)"
	@$(HOSTCC) $(HOSTCFLAGS) -MMD -MP -c $< -o $@

$(obj)/lexer.lex.o: $(obj)/lexer.lex.c $(obj)/parser.tab.h
	@echo "  HOSTCC  $(call quiet_path,$<)"
	@$(HOSTCC) $(HOSTCFLAGS) -MMD -MP -c $< -o $@

$(obj)/parser.tab.o: $(obj)/parser.tab.c $(obj)/parser.tab.h
	@echo "  HOSTCC  $(call quiet_path,$<)"
	@$(HOSTCC) $(HOSTCFLAGS) -MMD -MP -c $< -o $@

$(obj)/mconf.o: HOSTCFLAGS += $(shell test ! -f $(obj)/mconf-cflags || cat $(obj)/mconf-cflags)
$(obj)/lxdialog/%.o: HOSTCFLAGS += $(shell test ! -f $(obj)/mconf-cflags || cat $(obj)/mconf-cflags)
$(obj)/mconf.o $(addprefix $(obj)/lxdialog/,$(notdir $(filter lxdialog/%.o,$(mconf-objs)))): $(obj)/mconf-cflags

$(obj)/conf: $(addprefix $(obj)/,$(conf-objs))
	@echo "  HOSTLD  $(call quiet_path,$@)"
	@$(HOSTCC) $^ -o $@

$(obj)/mconf: $(addprefix $(obj)/,$(mconf-objs)) $(obj)/mconf-libs
	@echo "  HOSTLD  $(call quiet_path,$@)"
	@$(HOSTCC) $(filter %.o,$^) $$(cat $(obj)/mconf-libs) -o $@

clean:
	@test ! -d $(obj) || find $(obj) -type f \( -name '*.o' -o -name '*.d' -o -name conf \
		-o -name mconf -o -name 'parser.tab.*' -o -name 'lexer.lex.c' \
		-o -name 'mconf-cflags' -o -name 'mconf-libs' \) -delete

-include $(wildcard $(obj)/*.d $(obj)/lxdialog/*.d)
