include ../qsdk.config
include ../../platform.mak

# Convert asuswrt build environment variable to qca-sal-qos-test
TARGET_CC	:= $(CROSS_COMPILE)gcc
TARGET_CFLAGS	:= $(GCC_CPU_PARAM)
TARGET_CXXFLAGS	:= $(GCC_CPU_PARAM)
LINUX_KARCH	:= $(ARCH)
LINUX_DIR	:= $(LINUXDIR)
LINUX_VERSION	:= $(LINUX_KERNEL)
PKG_BUILD_DIR	:= $(shell pwd)/source
STAGING_DIR	:= $(STAGEDIR)

# Rules from Makefile in QSDK feed
TARGET_CFLAGS += -g -O0 -Werror -Wall \
         -fpic \
        -D__packed__= \
        -DIMAGE_APPS_PROC \
        -DFEATURE_Q_NO_SELF_QPTR \
        -DAEEINTERFACE_CPLUSPLUS \
        -DFEATURE_DIAG_MP_MASTER_APPS \
        -DFEATURE_MULTIPROCESSOR \
        -DFEATURE_IPQ_OPENWRT \
        -DFEATURE_NO_REX \
        -Dstrlcpy=g_strlcpy \
        -Dstrlcat=g_strlcat \
        -I$(STAGING_DIR)/usr/include

TARGET_LDFLAGS += \
        -L$(STAGING_DIR)/usr/lib \

TARGET_LDFLAGS += -pie -ljson-c

# Define variables for qca-sal-qos-test
BIN_LIST	:= $(PKG_BUILD_DIR)/salqostest
CONF_LIST	:= $(PKG_BUILD_DIR)/conf/qos_conf.json
TARGET_LDFLAGS	+= -L$(STAGEDIR)/usr/lib -lsalqos

all:
	$(call owrt_cmake, $(PKG_BUILD_DIR), $(PKG_BUILD_DIR))
	[ ! -e $(PKG_BUILD_DIR)/Makefile ] || $(MAKE) -j8 -C $(PKG_BUILD_DIR)

install:
	@mkdir -p $(addprefix $(INSTALLDIR)/, usr/sbin etc/sal_qos)
	install -m 755 $(foreach f,$(BIN_LIST),$(f)) $(INSTALLDIR)/usr/sbin
	install -m 755 $(foreach f,$(CONF_LIST),$(f)) $(INSTALLDIR)/etc/sal_qos
	$(STRIP) $(INSTALLDIR)/usr/sbin/*

clean:
	@find $(PKG_BUILD_DIR)/ \( -type f -o -type l \) -a \( -name "*.o" \
		-o -name "salqostest" \) \
		| xargs rm -fr
