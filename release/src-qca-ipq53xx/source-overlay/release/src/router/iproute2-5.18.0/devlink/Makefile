# SPDX-License-Identifier: GPL-2.0
include ../config.mk

DEVLINKOBJ = devlink.o mnlg.o
TARGETS += devlink
LDLIBS += -lm

all: $(TARGETS) $(LIBS)

devlink: $(DEVLINKOBJ) $(LIBNETLINK)
	$(QUIET_LINK)$(CC) $^ $(LDFLAGS) $(LDLIBS) -o $@

install: all
	for i in $(TARGETS); \
	do install -m 0755 $$i $(DESTDIR)$(SBINDIR); \
	done

clean:
	rm -f $(DEVLINKOBJ) $(TARGETS)
