# SPDX-License-Identifier: GPL-2.0+

TEST_ROOT := $(CURDIR)
O ?= $(TEST_ROOT)/out

case_makefiles := $(sort $(wildcard $(TEST_ROOT)/cases/*/Makefile))
case_dirs := $(patsubst %/Makefile,%,$(case_makefiles))
cases := $(notdir $(case_dirs))
host_cases := $(notdir $(patsubst %/Makefile,%,$(shell \
	rg -l '^TEST_TYPE[[:space:]]*:=[[:space:]]*host$$' $(case_makefiles))))
qemu_cases := $(filter-out $(host_cases),$(cases))

.PHONY: all host qemu list clean $(cases)

all: host qemu

host:
	@set -e; for case_name in $(host_cases); do \
		$(MAKE) --no-print-directory -C $(TEST_ROOT)/cases/$$case_name \
			TEST_OUT=$(O) run; \
	done

qemu:
	@set -e; for case_name in $(qemu_cases); do \
		$(MAKE) --no-print-directory -C $(TEST_ROOT)/cases/$$case_name \
			TEST_OUT=$(O) run; \
	done

$(cases):
	@$(MAKE) --no-print-directory -C $(TEST_ROOT)/cases/$@ TEST_OUT=$(O) run

list:
	@for case_name in $(cases); do printf '%s\n' "$$case_name"; done

clean:
	@$(RM) -r $(O)
