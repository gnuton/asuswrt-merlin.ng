# SPDX-License-Identifier: GPL-2.0
# Top level Makefile for iproute2

-include config.mk

ifeq ("$(origin V)", "command line")
VERBOSE = $(V)
endif
ifndef VERBOSE
VERBOSE = 0
endif

ifeq ($(VERBOSE),0)
MAKEFLAGS += --no-print-directory
endif

PREFIX?=/usr
SBINDIR?=/sbin
CONFDIR?=/etc/iproute2
NETNS_RUN_DIR?=/var/run/netns
NETNS_ETC_DIR?=/etc/netns
DATADIR?=$(PREFIX)/share
HDRDIR?=$(PREFIX)/include/iproute2
DOCDIR?=$(DATADIR)/doc/iproute2
MANDIR?=$(DATADIR)/man
ARPDDIR?=/var/lib/arpd
KERNEL_INCLUDE?=/usr/include
BASH_COMPDIR?=$(DATADIR)/bash-completion/completions

# Path to db_185.h include
DBM_INCLUDE:=$(DESTDIR)/usr/include

SHARED_LIBS = y

DEFINES= -DRESOLVE_HOSTNAMES -DLIBDIR=\"$(LIBDIR)\"
ifneq ($(SHARED_LIBS),y)
DEFINES+= -DNO_SHARED_LIBS
endif

DEFINES+=-DCONFDIR=\"$(CONFDIR)\" \
         -DNETNS_RUN_DIR=\"$(NETNS_RUN_DIR)\" \
         -DNETNS_ETC_DIR=\"$(NETNS_ETC_DIR)\"

#options for AX.25
ADDLIB+=ax25_ntop.o

#options for AX.25
ADDLIB+=rose_ntop.o

#options for mpls
ADDLIB+=mpls_ntop.o mpls_pton.o

#options for NETROM
ADDLIB+=netrom_ntop.o

HOSTCC = gcc
DEFINES += -D_GNU_SOURCE
# Turn on transparent support for LFS
DEFINES += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
CCOPTS = -O2 -pipe
WFLAGS := -Wall -Wstrict-prototypes  -Wmissing-prototypes
WFLAGS += -Wmissing-declarations -Wold-style-definition -Wformat=2

CFLAGS := $(WFLAGS) $(CCOPTS) -I../include -I../include/uapi $(DEFINES) $(CFLAGS)
YACCFLAGS = -d -t -v

SUBDIRS=lib ip tc bridge misc netem genl
ifeq ($(HAVE_MNL),y)
SUBDIRS += tipc devlink rdma dcb vdpa
endif

LIBNETLINK=../lib/libutil.a ../lib/libnetlink.a
LDLIBS += $(LIBNETLINK)

IPROUTE2_HDRS= \
        include/libnetlink.h

all: config.mk
	@set -e; \
	for i in $(SUBDIRS); \
	do echo; echo $$i; $(MAKE) -C $$i; done

.PHONY: clean clobber distclean check cscope version

help:
	@echo "Make Targets:"
	@echo " all                 - build binaries"
	@echo " clean               - remove products of build"
	@echo " distclean           - remove configuration and build"
	@echo " install             - install binaries on local machine"
	@echo " check               - run tests"
	@echo " cscope              - build cscope database"
	@echo " version             - update version"
	@echo ""
	@echo "Make Arguments:"
	@echo " V=[0|1]             - set build verbosity level"

config.mk:
	@if [ ! -f config.mk -o configure -nt config.mk ]; then \
		sh configure $(KERNEL_INCLUDE); \
	fi

install: all
	install -m 0755 -d $(DESTDIR)$(SBINDIR)
	install -m 0755 -d $(DESTDIR)$(CONFDIR)
	install -m 0755 -d $(DESTDIR)$(ARPDDIR)
	install -m 0755 -d $(DESTDIR)$(HDRDIR)
	@for i in $(SUBDIRS);  do $(MAKE) -C $$i install; done
	install -m 0644 $(shell find etc/iproute2 -maxdepth 1 -type f) $(DESTDIR)$(CONFDIR)
	install -m 0755 -d $(DESTDIR)$(BASH_COMPDIR)
	install -m 0644 bash-completion/tc $(DESTDIR)$(BASH_COMPDIR)
	install -m 0644 bash-completion/devlink $(DESTDIR)$(BASH_COMPDIR)
	install -m 0644 include/bpf_elf.h $(DESTDIR)$(HDRDIR)

version:
	echo "static const char version[] = \""`git describe --tags --long`"\";" \
		> include/version.h

clean:
	@for i in $(SUBDIRS) testsuite; \
	do $(MAKE) -C $$i clean; done

clobber:
	touch config.mk
	$(MAKE) clean
	rm -f config.mk cscope.*

distclean: clobber

check: all
	$(MAKE) -C testsuite
	$(MAKE) -C testsuite alltests
	@if command -v man >/dev/null 2>&1; then \
		echo "Checking manpages for syntax errors..."; \
		$(MAKE) -C man check; \
	else \
		echo "man not installed, skipping checks for syntax errors."; \
	fi

cscope:
	cscope -b -q -R -Iinclude -sip -slib -smisc -snetem -stc

stage: $(IPROUTE2_HDRS)
	@mkdir -p $(STAGEDIR)/usr/include
	install -m 644 $(foreach h,$^,$(h)) $(STAGEDIR)/usr/include
	@touch $@

.EXPORT_ALL_VARIABLES:
