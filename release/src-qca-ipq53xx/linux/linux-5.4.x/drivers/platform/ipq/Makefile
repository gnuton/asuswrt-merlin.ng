#
# Makefile for the IPQ specific device drivers.
#

obj-$(CONFIG_BOOTCONFIG_PARTITION) += bootconfig.o
ifneq ($(CONFIG_IPQ_FLASH_16M_PROFILE),y)
obj-y	 += sec-upgrade.o
endif
