include ../common.mak

CFLAGS += -Os -Wall $(EXTRACFLAGS)
CFLAGS += -I$(TOP)/shared -I$(SRCBASE)/include

LDFLAGS += -L$(TOP_PLATFORM)/nvram${BCMEX}${EX7} ${EXTRA_NV_LDFLAGS} -lnvram
LDFLAGS += -L$(TOP)/shared -L$(INSTALLDIR)/shared/usr/lib -lshared

OBJS = wpsaide.o

all: wpsaide

wpsaide: $(OBJS)
	@echo " [wpsaide] LD $@"
	$(CC) $(LDFLAGS) -o $@ $^

install: all
	@echo " [wpsaide] Installing as $(INSTALLDIR)/usr/sbin/wpsaide"
	@install -d $(INSTALLDIR)/usr/sbin
	@install -m 755 wpsaide $(INSTALLDIR)/usr/sbin/
	@$(STRIP) $(INSTALLDIR)/usr/sbin/wpsaide

%.o: %.c
	@echo " [wpsaide] CC $@"
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.so *.o wpsaide
