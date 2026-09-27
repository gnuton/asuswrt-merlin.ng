# SPDX-License-Identifier: GPL-2.0
include ../config.mk

CFLAGS += -I./include/uapi/
VDPAOBJ = vdpa.o
TARGETS += vdpa

all: $(TARGETS) $(LIBS)

vdpa: $(VDPAOBJ)
	$(QUIET_LINK)$(CC) $^ $(LDFLAGS) $(LDLIBS) -o $@

install: all
	for i in $(TARGETS); \
	do install -m 0755 $$i $(DESTDIR)$(SBINDIR); \
	done

clean:
	rm -f $(VDPAOBJ) $(TARGETS)
