include ../../common.mak

CFLAGS += -Wall -O

ifeq ($(RTCONFIG_DSL_TCLINUX),y)
CFLAGS += -DRTCONFIG_DSL_TCLINUX
endif

ifeq ($(RTCONFIG_VDSL),y)
CFLAGS += -DRTCONFIG_VDSL
endif

ifeq ($(RTCONFIG_BCMARM),y)
CFLAGS += $(EXTRA_FLAG)
endif

ifeq ($(RTCONFIG_BCMWL6),y)
CFLAGS += -I$(SRCBASE)/shared/bcmwifi/include
CFLAGS += -I$(SRCBASE)/common/include
endif

CFLAGS += -DASUS
CFLAGS += -I$(TOP)/shared -I$(SRCBASE)/include
LDFLAGS = -L$(TOP)/shared -lshared -L$(TOP)/nvram${BCMEX} -lnvram

ifeq ($(RTCONFIG_AMAS),y)
LDFLAGS += -L$(TOP)/json-c/.libs -ljson-c
endif

INSTALL = install

OBJS = req_dsl_drv.o

ifneq ($(wildcard ./prebuild),)
all:
	@-cp -f ./prebuild/req_dsl_drv .
else
all: req_dsl_drv
endif

req_dsl_drv: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

install: all
	$(STRIP) req_dsl_drv
	$(INSTALL) -d $(INSTALLDIR)/usr/sbin 
	$(INSTALL) req_dsl_drv $(INSTALLDIR)/usr/sbin

clean:
	rm -f req_dsl_drv *.o .*.depend

%.o: %.c .%.depend
	@echo " [req_dsl_drv] CC $@"
	@$(CC) $(CFLAGS) -o $@ -c $<

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)
