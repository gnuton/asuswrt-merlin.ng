#
# Linux wlceventd Makefile
#
# $Copyright ASUSTeK Corporation$
#
# $Id: Makefile 401201 2013-05-09 03:06:42Z kenlo $
#

include $(SRCBASE)/router/.config

CFLAGS	+= -I. -I$(TOP)/shared -I$(SRCBASE)/include -I$(SRCBASE)/common/include -Wall -Wno-implicit-function-declaration
CFLAGS	+= $(if $(WLAN_ComponentIncPath),$(WLAN_ComponentIncPath),$(addprefix -I,$(wildcard $(SRCBASE)/shared/bcmwifi/include)))
#CFLAGS	+= -g -DDEBUG
CFLAGS	+= -s -O2
LDFLAGS += -L$(TOP_PLATFORM)/nvram$(BCMEX)${EX7} ${EXTRA_NV_LDFLAGS} -lnvram
LDFLAGS += -L$(TOP)/shared -L$(INSTALLDIR)/shared/usr/lib -lshared
LDFLAGS += $(EXTRALDFLAGS)

ifeq ($(RTCONFIG_QTN),y)
CFLAGS += -I$(TOP)/libqcsapi_client -I$(TOP)/httpd -I./sysdeps/qtn
LDFLAGS += -L$(TOP)/libqcsapi_client -lqcsapi_client
endif

#ifdef BCMINTERNAL
ifneq ($(NO_BCMINTERNAL), 1)
CFLAGS	+= -DBCMINTERNAL
endif
CFLAGS	+= -DBCMDBG
#endif

ifeq ($(RTCONFIG_NOTIFICATION_CENTER),y)
CFLAGS  += -I$(TOP)/json-c
CFLAGS  += -I$(TOP)/sqlite
CFLAGS  += -I$(TOP)/nt_center/lib
CFLAGS  += -I$(TOP)/nt_center/actMail
CFLAGS  += -I$(TOP)/wlc_nt
CFLAGS  += -DASUSWRT_SDK
LDFLAGS += -L$(TOP)/json-c/.libs -ljson-c
LDFLAGS += -L$(TOP)/sqlite/.libs -lsqlite3 -lpthread -L$(TOP)/nt_center/lib -lnt -lm
LDFLAGS += -L$(TOP)/wlc_nt -lwlc_nt_client
endif

ifeq ($(RTCONFIG_CFGSYNC),y)
CFLAGS  += -I$(TOP)/cfg_mnt
CFLAGS  += -I$(TOP)/networkmap/api
LDFLAGS += -L$(TOP)/cfg_mnt -lcfgmnt
LDFLAGS += -L$(TOP)/networkmap/api/ -lnmpapi
endif

ifeq ($(RTCONFIG_SW_HW_AUTH), y)
CFLAGS += -I$(TOP)/sw-hw-auth
endif

ifneq ($(PLATFORM_HOSTAPD_DIR),)
CFLAGS += -I$(PLATFORM_HOSTAPD_DIR)/source/src
endif

vpath %.c $(SRCBASE)/shared $(WLAN_ComponentSrcDirs)

ifeq ($(wildcard $(SRCBASE)/router/hapdevent/*.c),)
all:
	-cp -f prebuild/hapdevent hapdevent
else
all: hapdevent
endif

OBJS = hapdevent_main.o 
ifneq ($(PLATFORM_ROUTER),)
OBJS += $(if $(wildcard $(STAGEDIR)/usr/lib/wpa_ctrl.o),$(STAGEDIR)/usr/lib/wpa_ctrl.o,\
	$(PLATFORM_HOSTAPD_DIR)/source/src/common/wpa_ctrl.o)
OBJS += $(if $(wildcard $(STAGEDIR)/usr/lib/os_unix.o),$(STAGEDIR)/usr/lib/os_unix.o,\
	$(PLATFORM_HOSTAPD_DIR)/source/src/utils/os_unix.o)
else
OBJS += ../hostapd-2.6/src/common/wpa_ctrl.o
OBJS += ../hostapd-2.6/src/utils/os_unix.o
endif

install: all
	install -d $(INSTALLDIR)/usr/sbin/
	install hapdevent $(INSTALLDIR)/usr/sbin/
	$(STRIP) $(INSTALLDIR)/usr/sbin/hapdevent

clean:
	rm -f *.o *.a *.so hapdevent

${OBJS}:

hapdevent: ${OBJS}
	$(CC) $(CFLAGS) $^ -o $@  $(LDFLAGS)

