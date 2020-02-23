#
# Linux Makefile
#
# Copyright (C) 2017, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#
# $Id: Makefile 341899 2012-06-29 04:06:38Z $
#

.PHONY: all clean install

ifeq ($(INSTALLDIR),)
$(error INSTALLDIR not defined)
endif

export SWSAVER = 2.6
SWSALIB = libairiq
SWSALIB_SO = $(SWSALIB).so.$(SWSAVER)
SIRPLIB = libairiq_sirp
SIRPLIB_SO = $(SIRPLIB).so.$(SWSAVER)

export AIRIQ_COMPONENTDIR =$(CURDIR)
$(info Building Air-IQ version $(SWSAVER) INSTALLDIR=$(INSTALLDIR))
$(info SRCBASE=$(SRCBASE) AIRIQ_COMPONENTDIR=$(AIRIQ_COMPONENTDIR) CURDIR=$(CURDIR))

all:
#create softlinks if it has been deleted in release package
ifneq ($(wildcard ./prebuilt/$(PLATFORM)/$(SWSALIB_SO)),)
ifeq ($(wildcard ./prebuilt/$(PLATFORM)/$(SWSALIB).so),)
	ln -sf ./$(SWSALIB_SO) ./prebuilt/$(PLATFORM)/$(SWSALIB).so
	ln -sf ./$(SIRPLIB_SO) ./prebuilt/$(PLATFORM)/$(SIRPLIB).so
endif
endif
ifneq ($(wildcard src/internal),)
	$(MAKE) -C src/internal
endif
	$(MAKE) -C src/app

clean:
ifneq ($(wildcard src/internal),)
	$(MAKE) -C src/internal clean
endif
	$(MAKE) -C src/app clean
	-rm -rf $(INSTALLDIR)/usr/sbin/airiq*
	-rm -rf $(INSTALLDIR)/lib/libairiq*
	-rm -rf $(INSTALLDIR)/etc/airiq*

install:
	# Install Air-IQ service and .cfg file
	install -d $(INSTALLDIR)/usr/sbin
	install -m 755 ./prebuilt/$(PLATFORM)/airiq_service $(INSTALLDIR)/usr/sbin
	install -m 555 ./prebuilt/$(PLATFORM)/airiq_service.cfg $(INSTALLDIR)/usr/sbin
	install -m 555 ./prebuilt/$(PLATFORM)/flash_policy.xml $(INSTALLDIR)/usr/sbin
	# Install Air-IQ libraries and .cfg file
	install -d $(INSTALLDIR)/lib
	install -m 755  ./prebuilt/$(PLATFORM)/$(SWSALIB_SO) $(INSTALLDIR)/lib/$(SWSALIB_SO)
	ln -sf ./$(SWSALIB_SO) $(INSTALLDIR)/lib/libairiq.so
	install -m 755  ./prebuilt/$(PLATFORM)/$(SIRPLIB_SO) $(INSTALLDIR)/lib/$(SIRPLIB_SO)
	ln -sf ./$(SIRPLIB_SO) $(INSTALLDIR)/lib/libairiq_sirp.so
	install -d $(INSTALLDIR)/etc
	install -m 555 ./prebuilt/$(PLATFORM)/airiq.cfg $(INSTALLDIR)/etc
ifdef BCA_CPEROUTER
	install -d $(INSTALL_DIR)/webs/wlrouter
	install ./www/* $(INSTALL_DIR)/webs/wlrouter
endif
	# Install Air-IQ app
	make -C src/app install
