include ../common.mak

CFLAGS += -Os -Wall $(EXTRACFLAGS)
CFLAGS += -I$(TOP)/shared -I$(SRCBASE)/include
CFLAGS += -I$(TOP)/json-c

LDFLAGS	+= -L$(TOP_PLATFORM)/nvram${BCMEX}${EX7} -lnvram 
LDFLAGS += -L$(TOP)/shared -lshared

OBJS = ntp.o 

ifeq ($(wildcard $(SRCBASE)/router/ntp/*.c),)
all:
	-cp -f prebuild/ntp ntp
else
all: ntp
endif


ntp: $(OBJS)
	@echo " [ntp] LD $@"
	$(CC) $(LDFLAGS) -o $@ $^

install: all
	@echo " [ntp] Installing as $(INSTALLDIR)/usr/sbin/ntp"
	@install -d $(INSTALLDIR)/usr/sbin
	@install -m 755 ntp $(INSTALLDIR)/usr/sbin/
	@$(STRIP) $(INSTALLDIR)/usr/sbin/ntp

%.o: %.c
	@echo " [ntp] CC $@"
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.so *.o ntp
