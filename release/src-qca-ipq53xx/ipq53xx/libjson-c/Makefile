include ../qsdk.config
include ../../platform.mak

# Convert asuswrt build environment variable to json-c
TARGET_CROSS	:= $(CROSS_COMPILE)
TARGET_CFLAGS	:= $(GCC_CPU_PARAM) $(if $(DEBUG),-g)
TARGET_CXXFLAGS	:= $(GCC_CPU_PARAM)
TARGET_LDFLAGS	:=
TARGET_CPPFLAGS	:=
PKG_BUILD_DIR	:= $(shell pwd)/source
PKG_FILES_DIR	:= $(shell pwd)/package/files
PKG_INSTALL_DIR	:= $(PKG_BUILD_DIR)/install
STAGING_DIR	:= $(STAGEDIR)

# Rules from Makefile in QSDK feed
CMAKE_HOST_OPTIONS += \
	-DDISABLE_EXTRA_LIBS=TRUE \
	-DBUILD_SHARED_LIBS=FALSE

CMAKE_OPTIONS += \
	-DDISABLE_EXTRA_LIBS=TRUE

# Define variables for json-c
LIB_LIST	:= $(wildcard $(PKG_BUILD_DIR)/libjson-c.so.5)

all:
	$(call owrt_cmake, $(PKG_BUILD_DIR), $(PKG_BUILD_DIR))
	[ ! -e $(PKG_BUILD_DIR)/Makefile ] || $(MAKE) -j8 -C $(PKG_BUILD_DIR)

install:
	@mkdir -p $(INSTALLDIR)/usr/lib
	install -m 755 $(foreach f,$(LIB_LIST),$(f)) $(INSTALLDIR)/usr/lib/
	$(STRIPX) $(INSTALLDIR)/usr/lib/*.so*

clean:
	@[ ! -e stage ] || rm -f stage
	@[ ! -e $(PKG_BUILD_DIR)/install ] || rm -fr $(PKG_BUILD_DIR)/install
	@find $(PKG_BUILD_DIR) -type d -a \( -name .deps \
		-o -name .libs \
		-o -name autom4te.cache \
		-o -name CMakeFiles \
		-o -name m4 \
		-o -name Testing \
		\) | xargs rm -fr
	@find $(PKG_BUILD_DIR) \( -type f -o -type l \) -a \( -name "*.a" \
		-o -name "*.cmake" \
		-o -name "*.lo" \
		-o -name "*.o" \
		-o -name "*.pc" \
		-o -name "*.so*" \
		-o -name aclocal.m4 \
		-o -name apps_config.h \
		-o -name check-version-script.sym \
		-o -name CMakeCache.txt \
		-o -name compile \
		-o -name config.guess \
		-o -name config.h \
		-o -name config.log \
		-o -name config.status \
		-o -name config.sub \
		-o -name configure \
		-o -name DartConfiguration.tcl \
		-o -name install-sh \
		-o -name json_config.h \
		-o -name json.h \
		-o -name json_parse \
		-o -name libtool \
		-o -name ltmain.sh \
		-o -name Makefile \
		-o -name Makefile.in \
		-o -name missing \
		-o -name stamp-h1 \
		-o -name test[124] \
		-o -name test[124]Formatted \
		-o -name test_cast \
		-o -name test_charcase \
		-o -name test_compare \
		-o -name test_deep_copy \
		-o -name test_double_serializer \
		-o -name test_float \
		-o -name test_int_add \
		-o -name test_json_pointer \
		-o -name test_locale \
		-o -name test_null \
		-o -name test_object_iterator \
		-o -name test_parse \
		-o -name test_parse_int64 \
		-o -name test_printbuf \
		-o -name testReplaceExisting \
		-o -name test_set_serializer \
		-o -name test_set_value \
		-o -name test_strerror \
		-o -name test_util_file \
		-o -name test_visit \
		\) | xargs rm -f

stage: $(LIB_LIST)
	@mkdir -p $(addprefix $(STAGEDIR)/,usr/lib)
	@rsync -avcHL --chmod=F664 $(foreach f,$(LIB_LIST),$(f)) $(STAGEDIR)/usr/lib
	@touch $@
