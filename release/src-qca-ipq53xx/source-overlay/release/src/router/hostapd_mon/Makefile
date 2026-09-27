include ../common.mak

CFLAGS += -Os -Wall $(EXTRACFLAGS)
CFLAGS += -I$(TOP)/shared -I$(SRCBASE)/include

LDFLAGS	+= -L$(TOP_PLATFORM)/nvram${BCMEX}${EX7} -lnvram
LDFLAGS += -L$(TOP)/shared -lshared

OBJS = hostapd_mon.o

ifeq ($(wildcard $(SRCBASE)/router/hostapd_mon/*.c),)
all:
	-cp -f prebuild/hostapd_mon hostapd_mon
else
all: hostapd_mon
endif

hostapd_mon: $(OBJS)
	@echo " [hostapd_mon] LD $@"
	$(CC) $(LDFLAGS) -o $@ $^

install: all
	@echo " [hostapd_mon] Installing as $(INSTALLDIR)/usr/sbin/hostapd_mon"
	@install -d $(INSTALLDIR)/usr/sbin
	@install -m 755 hostapd_mon $(INSTALLDIR)/usr/sbin/
	@$(STRIP) $(INSTALLDIR)/usr/sbin/hostapd_mon

%.o: %.c
	@echo " [hostapd_mon] CC $@"
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.so *.o hostapd_mon
