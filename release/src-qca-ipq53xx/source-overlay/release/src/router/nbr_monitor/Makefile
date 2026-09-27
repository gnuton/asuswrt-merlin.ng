include ../common.mak

CFLAGS += -Os -Wall $(EXTRACFLAGS)
CFLAGS += -I$(TOP)/shared -I$(SRCBASE)/include
CFLAGS += -I$(TOP)/json-c

LDFLAGS += -L$(TOP)/json-c/.libs -ljson-c
LDFLAGS	+= -L$(TOP_PLATFORM)/nvram${BCMEX}${EX7} -lnvram 
LDFLAGS += -L$(TOP)/shared -lshared

OBJS = nbr_monitor.o

ifeq ($(wildcard $(SRCBASE)/router/nbr_monitor/*.c),)
all:
	-cp -f prebuild/nbr_monitor nbr_monitor
else
all: nbr_monitor
endif


nbr_monitor: $(OBJS)
	@echo " [nbr_monitor] LD $@"
	$(CC) $(LDFLAGS) -o $@ $^

install: all
	@echo " [nbr_monitor] Installing as $(INSTALLDIR)/usr/sbin/nbr_monitor"
	@install -d $(INSTALLDIR)/usr/sbin
	@install -m 755 nbr_monitor $(INSTALLDIR)/usr/sbin/
	@$(STRIP) $(INSTALLDIR)/usr/sbin/nbr_monitor

%.o: %.c
	@echo " [nbr_monitor] CC $@"
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.so *.o nbr_monitor
