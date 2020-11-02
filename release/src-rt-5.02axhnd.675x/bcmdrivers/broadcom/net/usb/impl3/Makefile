# File: bcmdrivers/broadcom/net/usb/Makefile
#
# Makefile for the BCM63xx USB driver
#

obj-$(CONFIG_BCM_USB) += bcm_usb_dev.o
bcm_usb_dev-objs += bcmusbdesc.o bcmusb.o bcmrndis.o

EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMDRIVER_PRIV_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PRIV_PATH)/$(BRCM_BOARD) -I$(INC_BRCMBOARDPARMS_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -DUSE_BH -DHIGH_SPEED
EXTRA_CFLAGS += -Werror -Wfatal-errors

-include $(TOPDIR)/Rules.make
