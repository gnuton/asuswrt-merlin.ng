include ../common.mak

CFLAGS += -Os -Wall $(EXTRACFLAGS) -fPIC
CFLAGS += -I. -I$(TOP)/shared -I$(SRCBASE)/include
ifeq ($(RTCONFIG_BCMARM),y)
CFLAGS += -I$(SRCBASE)/shared/bcmwifi/include -DTYPEDEF_FLOAT_T
endif

LDFLAGS += $(EXTRALDFLAGS) -L$(TOP)/shared -lshared
ifeq ($(RTCONFIG_BCM_502L07P2),y)
LDFLAGS += -L$(TOP_PLATFORM)/ethctl_lib -lethctl
endif
LDFLAGS += -L$(TOP_PLATFORM)/nvram$(BCMEX)${EX7} ${EXTRA_NV_LDFLAGS} -lnvram

OBJS = auto_det.o config.o

ifeq ($(RTCONFIG_DSL_BCM),y)
OBJS += bcm.o
endif

INSTALL = install

ifeq ($(wildcard ./*.c),)
all:
	cp -f ./prebuild/auto_det .
else
all: auto_det
endif

auto_det: $(OBJS)
	@echo " [auto_det] LD $@"
	@$(CC) $(LDFLAGS) -o $@ $^

install: all
	@echo " [auto_det] Installing as $(INSTALLDIR)/usr/sbin/auto_det"
	@$(INSTALL) -d $(INSTALLDIR)/usr/sbin
	@$(INSTALL) -m 755 auto_det $(INSTALLDIR)/usr/sbin/
	@$(STRIP) $(INSTALLDIR)/usr/sbin/auto_det

clean:
	rm -f *.o *.so *.a .*.depend *.prep

%.o: %.c .%.depend
	@echo " [auto_det] CC $@"
	@$(CC) $(CFLAGS) -o $@ -c $<

%.o: prebuild/%.o
	@echo " [auto_det] cp $@"
	@cp -f $< $@

.depend: $(OBJS:%.o=%.c)
	@$(CC) $(CFLAGS) -M $^ > .depend

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

%.prep: %.c
	@$(CC) $(CFLAGS) -E $< > $@

-include $(OBJS:%.o=.%.depend)
