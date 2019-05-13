include ../common.mak

CFLAGS	= -Os -Wall $(EXTRACFLAGS) #-mips32
CFLAGS	+= -I$(SRCBASE)/include -I$(TOP)/shared -DRTCONFIG_ISP_METER -DDEBUG
LDFLAGS	= $(EXTRALDFLAGS)

ifeq ($(RTCONFIG_BCMARM),y)
CFLAGS += -I$(SRCBASE)/common/include
endif

ifeq ($(RTCONFIG_BCMWL6),y)
CFLAGS += -I$(SRCBASE)/shared/bcmwifi/include
CFLAGS += -I$(SRCBASE)/common/include
endif

ifeq ($(RTCONFIG_DSL_TCLINUX),y)
OBJS = spectrum-tclinux.o
else
OBJS = spectrum.o
endif

ifeq ($(wildcard $(TOP)/spectrum/*.c),)
all:
	-cp -f $(TOP)/spectrum/prebuild/spectrum spectrum
else
all: spectrum
endif

spectrum: $(OBJS)
	@echo " [spectrum] CC -o $@"
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -L../nvram$(BCMEX) -lnvram -L../shared -lshared

	$(SIZECHECK)
	$(CPTMP)

clean:
	rm -f spectrum .*.depend
	rm -f *.o

install: all
	@echo " [spectrum] Installing..."
	install -D spectrum $(INSTALLDIR)/bin/spectrum
	$(STRIP) $(INSTALLDIR)/bin/spectrum
	chmod 0500 $(INSTALLDIR)/bin/spectrum

%.o: %.c .%.depend
	@echo " [spectrum] CC $@"
	@$(CC) $(CFLAGS) -c $<

.depend: $(OBJS:%.o=%.c)
	@$(CC) $(CFLAGS) -M $^ > .depend

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)
