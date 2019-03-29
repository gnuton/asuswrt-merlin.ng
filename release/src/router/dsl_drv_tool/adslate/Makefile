include ../../common.mak

ifeq ($(BUILD_NAME), DSL-N55U)
CFLAGS += -DDSL_N55U=1
CFLAGS += -DDSL_N55U_ANNEX_A=1
endif
ifeq ($(BUILD_NAME), DSL-N55U-B)
CFLAGS += -DDSL_N55U=1
CFLAGS += -DDSL_N55U_ANNEX_B=1
endif
ifeq ($(BUILD_NAME), DSL-AC68U)
CFLAGS += -DDSL_AC68U
endif

ifeq ($(RTCONFIG_DSL),y)
CFLAGS += -DRTCONFIG_DSL
endif

ifeq ($(RTCONFIG_DSL_TCLINUX),y)
CFLAGS += -DRTCONFIG_DSL_TCLINUX
endif

ifeq ($(RTCONFIG_VDSL),y)
CFLAGS += -DRTCONFIG_VDSL
endif

ifeq ($(ANNEX_B),y)
CFLAGS += -DRTCONFIG_DSL_ANNEX_B
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

ifeq ($(CONFIG_RALINK),y)
CFLAGS += -I$(LINUXDIR)/drivers/net/raeth.dsl
CFLAGS += -DRTCONFIG_RALINK
endif
CFLAGS += -I$(LINUXDIR)/include

LDFLAGS = -L$(TOP)/shared -lshared -L$(TOP)/nvram${BCMEX} -lnvram


INSTALL = install

OBJS = adslate.o

ifeq ($(CONFIG_RALINK),y)
OBJS += ra_reg_rw_ate.o
else
OBJS += switch_control.o
endif

ifneq ($(wildcard ./prebuild),)
all:
	@-cp -f ./prebuild/adslate .
else
all: adslate
endif

adslate: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

install: all
	$(STRIP) adslate
	$(INSTALL) -d $(INSTALLDIR)/usr/sbin 
	$(INSTALL) adslate $(INSTALLDIR)/usr/sbin

clean:
	rm -f adslate *.o .*.depend

%.o: %.c .%.depend
	@echo " [adslate] CC $@"
	@$(CC) $(CFLAGS) -o $@ -c $<

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)
