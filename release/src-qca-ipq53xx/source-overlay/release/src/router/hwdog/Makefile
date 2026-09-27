include ../common.mak

CFLAGS += -Os -Wall $(EXTRACFLAGS) 

LDFLAGS = 

OBJS = hwdog.o

ifeq ($(wildcard $(SRCBASE)/router/hwdog/*.c),)
all:
	-cp -f prebuild/hwdog hwdog
else
all: hwdog
endif

hwdog: $(OBJS)
	@echo " [hwdog] LD $@"
	$(CC) $(LDFLAGS) -o $@ $^

install: all
	@echo " [hwdog] Installing as $(INSTALLDIR)/usr/sbin/hwdog"
	@install -d $(INSTALLDIR)/usr/sbin
	@install -m 755 hwdog $(INSTALLDIR)/usr/sbin/
	@$(STRIP) $(INSTALLDIR)/usr/sbin/hwdog

%.o: %.c
	@echo " [hwdog] CC $@"
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.so *.o hwdog
