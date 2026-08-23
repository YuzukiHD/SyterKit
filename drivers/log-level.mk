# SPDX-License-Identifier: GPL-2.0+

# Convert a driver's Kconfig log-level choice to a preprocessor override.
# The global choice intentionally produces no flag and keeps log.h defaults.
driver-log-flags = \
	$(if $(CONFIG_$(1)_LOG_MUTE),-DDRIVER_LOG_LEVEL=0) \
	$(if $(CONFIG_$(1)_LOG_ERROR),-DDRIVER_LOG_LEVEL=1) \
	$(if $(CONFIG_$(1)_LOG_WARNING),-DDRIVER_LOG_LEVEL=2) \
	$(if $(CONFIG_$(1)_LOG_INFO),-DDRIVER_LOG_LEVEL=3) \
	$(if $(CONFIG_$(1)_LOG_DEBUG),-DDRIVER_LOG_LEVEL=4) \
	$(if $(CONFIG_$(1)_LOG_TRACE),-DDRIVER_LOG_LEVEL=5) \
	$(if $(CONFIG_$(1)_LOG_BACKTRACE),-DDRIVER_LOG_LEVEL=6)
