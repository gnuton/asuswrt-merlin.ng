include ../common.mak
include $(SRCBASE)/.config
include $(SRCBASE)/router/.config

CFLAGS += -Os -Wall $(EXTRACFLAGS) -fPIC
CFLAGS += -I. -I$(TOP)/shared -I$(SRCBASE)/include
ifeq ($(RTCONFIG_BCMARM),y)
CFLAGS += -I$(SRCBASE)/shared/bcmwifi/include -DTYPEDEF_FLOAT_T
endif

LDFLAGS += $(EXTRALDFLAGS) -L$(TOP)/shared -lshared
LDFLAGS += -L$(TOP_PLATFORM)/nvram$(BCMEX)${EX7} ${EXTRA_NV_LDFLAGS} -lnvram

OBJS = dsl_diag.o config.o

ifeq ($(RTCONFIG_DSL_BCM),y)
OBJS += bcm.o
endif

INSTALL = install

ifeq ($(wildcard ./*.c),)
all:
	cp -f ./prebuild/dsl_diag .
else
all: dsl_diag
endif

dsl_diag: $(OBJS)
	@echo " [dsl_diag] LD $@"
	@$(CC) $(LDFLAGS) -o $@ $^

install: all
	@echo " [dsl_diag] Installing as $(INSTALLDIR)/usr/sbin/dsl_diag"
	@$(INSTALL) -d $(INSTALLDIR)/usr/sbin
	@$(INSTALL) -m 755 dsl_diag $(INSTALLDIR)/usr/sbin/
	@$(STRIP) $(INSTALLDIR)/usr/sbin/dsl_diag

clean:
	rm -f *.o *.so *.a .*.depend *.prep

%.o: %.c .%.depend
	@echo " [dsl_diag] CC $@"
	@$(CC) $(CFLAGS) -o $@ -c $<

%.o: prebuild/%.o
	@echo " [dsl_diag] cp $@"
	@cp -f $< $@

.depend: $(OBJS:%.o=%.c)
	@$(CC) $(CFLAGS) -M $^ > .depend

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

%.prep: %.c
	@$(CC) $(CFLAGS) -E $< > $@

-include $(OBJS:%.o=.%.depend)
