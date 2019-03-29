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
CFLAGS += -DTYPEDEF_FLOAT_T
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


ifeq ($(RTCONFIG_DSL_TCLINUX),y)
LDFLAGS += -lpthread
LDFLAGS += -lm
endif

INSTALL = install

OBJS = tp_init_main.o tp_msg.o scanner.o respipc.o nvram.o

ifeq ($(CONFIG_RALINK),y)
OBJS += ra_reg_rw.o
endif

ifeq ($(RTCONFIG_DSL_TCLINUX),y)
OBJS += dumptcc.o
OBJS += runtcc.o
OBJS += monitor.o dla.o
OBJS += ginptry.o
endif

ifneq ($(wildcard ./prebuild),)
all:
	@-cp -f ./prebuild/tp_init .
else
all: tp_init
endif

tp_init: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

install: all
	$(STRIP) tp_init
	$(INSTALL) -d $(INSTALLDIR)/usr/sbin 
	$(INSTALL) tp_init $(INSTALLDIR)/usr/sbin

clean:
	rm -f tp_init *.o .*.depend

%.o: %.c .%.depend
	@echo " [tp_init] CC $@"
	@$(CC) $(CFLAGS) -o $@ -c $<

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)
