include ../../common.mak

ifeq ($(RTCONFIG_DSL),y)
CFLAGS += -DRTCONFIG_DSL
endif

ifeq ($(ANNEX_B),y)
CFLAGS += -DRTCONFIG_DSL_ANNEX_B
endif

ifeq ($(RTCONFIG_DSL_TCLINUX),y)
CFLAGS += -DRTCONFIG_DSL_TCLINUX
endif

ifeq ($(RTCONFIG_BCMARM),y)
CFLAGS += $(EXTRA_FLAG)
CFLAGS += -I$(SRCBASE)/common/include
endif

ifeq ($(RTCONFIG_BCMWL6),y)
CFLAGS += -I$(SRCBASE)/shared/bcmwifi/include
CFLAGS += -I$(SRCBASE)/common/include
endif

CFLAGS += -Wall -O

CFLAGS += -DASUS
CFLAGS += -I$(TOP)/shared -I$(SRCBASE)/include
LDFLAGS = -L$(TOP)/shared -lshared -L$(TOP)/nvram${BCMEX} -lnvram


INSTALL = install

ifeq ($(RTCONFIG_DSL_TCLINUX),y)
OBJS = auto_det_tclinux.o
else
OBJS = auto_det.o discover.o
endif

ifneq ($(wildcard ./prebuild),)
all:
	@-cp -f ./prebuild/auto_det .
else
all: auto_det
endif

auto_det: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

install: all
	$(STRIP) auto_det
	$(INSTALL) -d $(INSTALLDIR)/usr/sbin 
	$(INSTALL) auto_det $(INSTALLDIR)/usr/sbin

clean:
	rm -f auto_det *.o .*.depend

%.o: %.c .%.depend
	@echo " [auto_det] CC $@"
	@$(CC) $(CFLAGS) -o $@ -c $<

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)
