include ../qsdk.config
include ../../platform.mak

# Convert asuswrt build environment variable to libnl-tiny-0.1
TARGET_CROSS	:= $(CROSS_COMPILE)
TARGET_CFLAGS	:= $(GCC_CPU_PARAM) $(if $(DEBUG),-g)
TARGET_CXXFLAGS	:= $(GCC_CPU_PARAM)
TARGET_LDFLAGS	:=
TARGET_CPPFLAGS	:=
PKG_BUILD_DIR	:= $(shell pwd)/source
PKG_FILES_DIR	:= $(shell pwd)/package/files
PKG_INSTALL_DIR	:= $(PKG_BUILD_DIR)/install
STAGING_DIR	:= $(STAGEDIR)
PKG_JOBS	:= -j8
PKG_VERSION	:= 5.1.5

# Rules from Makefile in QSDK feed
FPIC		:= -fPIC
TARGET_CFLAGS += $(FPIC)

# Define variables for libnl-tiny-0.1
HDR_LIST	:= $(wildcard $(PKG_BUILD_DIR)/include/*)
LIB_LIST	:= $(wildcard $(PKG_BUILD_DIR)/libnl-tiny.so*)
ALIB_LIST	:= $(wildcard $(PKG_BUILD_DIR)/libnl-tiny.a)
PC_LIST		:= $(PKG_BUILD_DIR)/libnl-tiny.pc

all:
	$(call owrt_cmake, $(PKG_BUILD_DIR), $(PKG_BUILD_DIR))
	[ ! -e $(PKG_BUILD_DIR)/Makefile ] || $(MAKE) -j8 -C $(PKG_BUILD_DIR)

install:
	@mkdir -p $(INSTALLDIR)/usr/lib
	rsync -avcH --chmod=F755 $(foreach f,$(LIB_LIST),$(f)) $(INSTALLDIR)/usr/lib/
	$(STRIPX) $(INSTALLDIR)/usr/lib/*.so

clean:
	@[ ! -e stage ] || rm -f stage
	@[ ! -e $(PKG_BUILD_DIR)/install ] || rm -fr $(PKG_BUILD_DIR)/install
	@[ ! -e $(PKG_BUILD_DIR)/CMakeFiles ] || rm -fr $(PKG_BUILD_DIR)/CMakeFiles
	@find $(PKG_BUILD_DIR) \( -type f -o -type l \) -a \( -name ".*.o.cmd" \
		-o -name ".*.ko.cmd" \
		-o -name "*.a" \
		-o -name "*.o" \
		-o -name "*.so" \
		-o -name "*.ko" \
		-o -name "modules.order" \
		-o -name ".tmp_versions" \
		-o -name "Module.symvers" \
		-o -name "*.mod.c" \
		-o -name "libnl-tiny.so*" \
		-o -name "libnl-tiny.pc" \
		-o -name "lua" \
		-o -name "luac" \
		-o -name Makefile \
		-o -name CMakeCache.txt \
		-o -name cmake_install.cmake \) \
		| xargs rm -fr

stage: $(LIB_LIST) $(ALIB_LIST) $(HDR_LIST) $(PC_LIST)
	@mkdir -p $(addprefix $(STAGEDIR)/,usr/lib usr/include usr/lib/pkgconfig)
	@rsync -avcH --chmod=F664 $(foreach f,$(HDR_LIST),$(f)) $(STAGEDIR)/usr/include
	@rsync -avcH --chmod=F755 $(foreach f,$(ALIB_LIST) $(LIB_LIST),$(f)) $(STAGEDIR)/usr/lib
	@rsync -avcH --chmod=F664 $(foreach f,$(PC_LIST),$(f)) $(STAGEDIR)/usr/lib/pkgconfig
	@touch $@
