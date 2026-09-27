BUILD_NAME ?= $(shell echo $(MAKECMDGOALS) | tr a-z A-Z)
export _BUILD_NAME_ := $(addprefix _,$(addsuffix _,$(BUILD_NAME)))

ifeq ($(_BUILD_NAME_),_TUF-BE6500_)
ALT_BUILD_NAMES := TUF-BE9400
endif

# related path to platform specific software package
export PLATFORM_ROUTER := ipq53xx

#### 1st round have no any exported value in target.mak
MUSL32_MODEL_LIST := BD4D5 ETJ

ifeq ($(filter-out n,$(and $(MUSL64),$(MUSL32))),)
ifneq ($(findstring $(BUILD_NAME),$(MUSL32_MODEL_LIST)),)
export MUSL32 := y
else
export MUSL64 := y
endif
endif

export LINUXDIR := $(SRCBASE)/linux/linux-5.4.x

export BUILD := $(shell (gcc -dumpmachine))
export KERNEL_BINARY=$(LINUXDIR)/vmlinux

ifeq ($(MUSL32),y)
$(shell ln -sf config_base32 $(LINUXDIR)/config_base)
$(shell ln -sf musl_qsdk32.config $(SRCBASE)/$(PLATFORM_ROUTER)/qsdk.config)
export LOADADDR := 40008000
export PLATFORM := arm-musl
export TOOLS := /opt/openwrt-gcc750_musl1124.arm
export CROSS_COMPILE := $(TOOLS)/bin/arm-openwrt-linux-
export READELF := $(TOOLS)/bin/arm-openwrt-linux-muslgnueabi-readelf

export RTVER := $(if $(wildcard $(TOOLS)/lib/librt-*.so),$(patsubst librt-%.so,%,$(shell basename $(wildcard $(TOOLS)/lib/librt-*.so))))
export CROSS_COMPILER := $(CROSS_COMPILE)
export CONFIGURE := ./configure --host=arm-linux --build=$(BUILD)
export HOSTCONFIG := linux-armv4	# Configure script of openssl
# ARCH is used for linux kernel and some other else
export ARCH := arm
export HOST := arm-linux
ifeq ($(EXTRACFLAGS),)
export EXTRACFLAGS := -DBCMWPA2 -fno-delete-null-pointer-checks -march=armv7-a -mlittle-endian -fno-strict-aliasing -fno-common
endif
else ifeq ($(MUSL64),y)
$(shell ln -sf config_base64 $(LINUXDIR)/config_base)
$(shell ln -sf musl_qsdk64.config $(SRCBASE)/$(PLATFORM_ROUTER)/qsdk.config);
export LOADADDR := 40080000
export PLATFORM := aarch64-musl
export TOOLS := /opt/openwrt-gcc750_musl1124.aarch64
export CROSS_COMPILE := $(TOOLS)/bin/aarch64-openwrt-linux-musl-
export READELF := $(TOOLS)/bin/aarch64-openwrt-linux-musl-readelf

export RTVER := $(if $(wildcard $(TOOLS)/lib/librt-*.so),$(patsubst librt-%.so,%,$(shell basename $(wildcard $(TOOLS)/lib/librt-*.so))))
export CROSS_COMPILER := $(CROSS_COMPILE)
export CONFIGURE := ./configure --host=aarch64-linux-gnu --build=$(BUILD)
export HOSTCONFIG := linux-aarch64	# Configure script of openssl
# ARCH is used for linux kernel and some other else
export ARCH := arm64
export HOST := aarch64-linux
ifeq ($(EXTRACFLAGS),)
export EXTRACFLAGS := -DBCMWPA2 -fno-delete-null-pointer-checks -march=armv8-a -mcpu=cortex-a53+crypto
endif
export DTB := "qcom/$(DTB)"
else
$(error "MUSL32 or MUSL64??")
endif
include $(SRCBASE)/$(PLATFORM_ROUTER)/qsdk.config

export KERNELCC ?= $(CROSS_COMPILE)gcc
export KERNELLD ?= $(CROSS_COMPILE)ld
export KARCH := $(firstword $(subst -, ,$(shell $(KERNELCC) -dumpmachine)))

# Kernel load address and entry address
export ENTRYADDR := $(LOADADDR)

# OpenWRT's toolchain needs STAGING_DIR environment variable that points to top directory of toolchain.
export STAGING_DIR=$(TOOLS)
EXTRA_CFLAGS := -DLINUX26 -DCONFIG_QCA -pipe -DDEBUG_NOISY -DDEBUG_RCTEST
EXTRA_CFLAGS += -D_GNU_SOURCE -D_BSD_SOURCE

export CONFIG_LINUX26=y
export CONFIG_QCA=y

EXTRA_CFLAGS += -DLINUX30
export CONFIG_LINUX30=y

export KERNEL_MAKE:=$(MAKE) -j8 -C "$(LINUXDIR)" CROSS_COMPILE="$(CROSS_COMPILE)" ARCH="$(ARCH)"

# Reference to QSDK/include/cmake.mk
# Set below variable in Makefile and then use the script if need.
# CMAKE_OPTIONS
#
# $(1): CMAKE_BINARY_DIR
# $(2): CMAKE_SOURCE_DIR
define owrt_cmake
$(if $(1),,$(error empty CMAKE_BINARY_DIR!))
$(if $(2),,$(error empty CMAKE_SOURCE_DIR!))
[ -e $(1) ] || mkdir -p $(1) ; \
(cd $(1); \
	CFLAGS="$(TARGET_CFLAGS) $(EXTRA_CFLAGS)" \
	CXXFLAGS="$(TARGET_CXXFLAGS) $(EXTRA_CXXFLAGS)" \
	LDFLAGS="$(TARGET_LDFLAGS) $(EXTRA_LDFLAGS)" \
	cmake \
		-DCMAKE_SYSTEM_NAME=Linux \
		-DCMAKE_SYSTEM_VERSION=1 \
		-DCMAKE_SYSTEM_PROCESSOR=$(ARCH) \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_FLAGS_RELEASE="-DNDEBUG" \
		-DCMAKE_CXX_FLAGS_RELEASE="-DNDEBUG" \
		-DCMAKE_C_COMPILER="$(CROSS_COMPILE)gcc" \
		-DCMAKE_C_COMPILER_ARG1="" \
		-DCMAKE_CXX_COMPILER="$(CROSS_COMPILE)g++" \
		-DCMAKE_CXX_COMPILER_ARG1="" \
		-DCMAKE_ASM_COMPILER="$(CROSS_COMPILE)gcc" \
		-DCMAKE_ASM_COMPILER_ARG1="" \
		-DCMAKE_EXE_LINKER_FLAGS:STRING="-L$(STAGEDIR)/usr/lib -L$(STAGEDIR)/lib -L$(TOOLS)/usr/lib -L$(TOOLS)/lib -fPIC -znow -zrelro" \
		-DCMAKE_MODULE_LINKER_FLAGS:STRING="-L$(STAGEDIR)/usr/lib -L$(STAGEDIR)/lib -L$(TOOLS)/usr/lib -L$(TOOLS)/lib -fPIC -znow -zrelro -Wl,-Bsymbolic-functions" \
		-DCMAKE_SHARED_LINKER_FLAGS:STRING="-L$(STAGEDIR)/usr/lib -L$(STAGEDIR)/lib -L$(TOOLS)/usr/lib -L$(TOOLS)/lib -fPIC -znow -zrelro -Wl,-Bsymbolic-functions" \
		-DCMAKE_AR="$(CROSS_COMPILE)gcc-ar" \
		-DCMAKE_NM="$(CROSS_COMPILE)gcc-nm" \
		-DCMAKE_RANLIB="$(CROSS_COMPILE)gcc-ranlib" \
		-DCMAKE_FIND_ROOT_PATH="$(STAGEDIR)/usr;$(TOOLS)" \
		-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=BOTH \
		-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
		-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
		-DCMAKE_STRIP=: \
		-DCMAKE_INSTALL_PREFIX=/usr \
		-DDL_LIBRARY=$(STAGEDIR) \
		-DCMAKE_PREFIX_PATH=$(STAGEDIR) \
		-DCMAKE_SKIP_RPATH=TRUE \
		$(CMAKE_OPTIONS) \
	$(2) \
)
endef

SWITCH_CHIP_ID_POOL =					\
	"QCA8386"

define platformRouterOptions
	$(call scfg, $(1), RTCONFIG_QCA, y)
	$(call scfg, $(1), RTCONFIG_SPF12_5_QSDK, y)
	$(call scfg, $(1), RTCONFIG_QCA_BECHIP, y)
	$(call scfg, $(1), RTCONFIG_CFG80211, y)
	$(call scfg, $(1), RTCONFIG_GLOBAL_INI, y)
	$(call scfg, $(1), RTCONFIG_HW_DOG, y)
	$(call scfg, $(1), RTCONFIG_DUAL_TRX2, $(DUAL_TRX))
	$(call scfg, $(1), RTCONFIG_SOC_IPQ53XX, y)
	$(call scfg, $(1), RTCONFIG_SINGLE_HOSTAPD, y)
	$(call scfg, $(1), RTCONFIG_MFP, y)
	$(call scfg, $(1), RTCONFIG_PAGECACHE_RATIO, n)
	$(call scfg, $(1), RTCONFIG_QCA_ARM, y)
	$(call scfg, $(1), RTCONFIG_32BYTES_ODMPID, y)
	$(call scfg, $(1), RTCONFIG_FITFDT, n)
	$(call scfg, $(1), RTCONFIG_QCA_VAP_LOCALMAC, n)
	$(call scfg, $(1), RTCONFIG_AVOID_TZ_ENV, y)
	if [ "$(WIFI_CHIP)" = "QCN6274" ] ; then \
		$(call scfg, $(1), RTCONFIG_WIFI_IPQ53XX_QCN6274, y); \
		$(call scfg, $(1), RTCONFIG_VHT80_80, n); \
		$(call scfg, $(1), RTCONFIG_QCA_BIGRATE_WIFI, y); \
	fi
	if [ "$(WIFI_CHIP)" = "QCN6274_5P6" ] ; then \
		$(call scfg, $(1), RTCONFIG_WIFI_IPQ53XX_QCN6274_5P6, y); \
		$(call scfg, $(1), RTCONFIG_VHT80_80, n); \
		$(call scfg, $(1), RTCONFIG_QCA_BIGRATE_WIFI, y); \
	fi
	$(call scfg, $(1), RTCONFIG_VHT160, $(BW160M))
	$(call scfg, $(1), RTCONFIG_BW160M, $(BW160M))
	$(call scfg, $(1), RTCONFIG_BW240M, $(BW240M))
	for chip in $(SWITCH_CHIP_ID_POOL) ; do \
		if [ "$(SWITCH_CHIP)" = "$${chip}" ] ; then \
			$(call scfg, $(1), RTCONFIG_SWITCH_$${chip}, y); \
		else \
			$(call scfg, $(1), RTCONFIG_SWITCH_$${chip}, n); \
		fi; \
	done
	$(call scfg, $(1), RTCONFIG_TEST_BOARDDATA_FILE, $(TEST_BDF)); \
	if [ -n "$(wildcard $(TOOLS)/lib/ld-musl*)" ] ; then \
		$(call scfg, $(1), RTCONFIG_MUSL_LIBC, y); \
	fi
	if [ "$(BUILD_NAME)" = "BD4D5" -o "$(BUILD_NAME)" = "BD4_OD" ]; then \
		sed -i "/RTCONFIG_ZENWIFI_RGBLED\>/d" $(1); \
		echo "RTCONFIG_ZENWIFI_RGBLED=y" >>$(1); \
		sed -i "/RTCONFIG_GPIOX3_RGBLED\>/d" $(1); \
		echo "RTCONFIG_GPIOX3_RGBLED=y" >>$(1); \
	fi
endef

define platformBusyboxOptions
	$(call scfg, $(1), CONFIG_FEATURE_TOP_SMP_CPU, y)
	$(call scfg, $(1), CONFIG_FEATURE_TOP_DECIMALS, y)
	$(call scfg, $(1), CONFIG_FEATURE_TOP_SMP_PROCESS, y)
	$(call scfg, $(1), CONFIG_FEATURE_TOPMEM, y)
	$(call scfg, $(1), CONFIG_FEATURE_SHOW_THREADS, y)
	$(call scfg, $(1), CONFIG_DEVMEM, y)
	$(call scfg, $(1), CONFIG_TFTP, y)
	$(call scfg, $(1), CONFIG_FEATURE_TFTP_GET, y)
	$(call scfg, $(1), CONFIG_FEATURE_TFTP_PUT, y)
	$(call scfg, $(1), CONFIG_SEQ, y)
endef

define platformKernelConfig
	$(call scfg, $(1), CONFIG_COMPAT, y)
	$(call scfg, $(1), CONFIG_KUSER_HELPERS, y)
	$(call scfg, $(1), CONFIG_BRIDGE_NETFILTER, y)
	$(call scfg, $(1), CONFIG_NETFILTER_XT_TARGET_TPROXY, m)
	$(call scfg, $(1), CONFIG_NF_CONNTRACK_CHAIN_EVENTS, y)
	$(call scfg, $(1), CONFIG_NETFILTER_XT_MATCH_PHYSDEV, y)
	$(call scfg, $(1), CONFIG_RTC_CLASS, y)
	$(call scfg, $(1), CONFIG_CRYPTO_CRC32, y)
	if [ -n "$(findstring ntfs3, $(NTFS))" ] ; then \
		$(call scfg, $(1), CONFIG_NTFS3_FS, y) ; \
		$(call scfg, $(1), CONFIG_NTFS3_64BIT_CLUSTER, n) ; \
		$(call scfg, $(1), CONFIG_NTFS3_LZX_XPRESS, y) ; \
		$(call scfg, $(1), CONFIG_NTFS3_FS_POSIX_ACL, n) ; \
	fi
	if [ "$(CONFIG_PACKAGE_kmod-qca-nss-ppe-gretap)" = "y" ] ; then \
		$(call scfg, $(1), CONFIG_NET_IPGRE, y); \
		$(call scfg, $(1), CONFIG_NET_IPGRE_BROADCAST, y); \
		$(call scfg, $(1), CONFIG_OPENVSWITCH_GRE, n); \
	fi
	if [ "$(CONFIG_PACKAGE_kmod-qca-nss-ppe-tunipip6)" = "y" ] ; then \
		$(call scfg, $(1), CONFIG_IPV6_TUNNEL, y); \
	fi
	if [ "$(CONFIG_PACKAGE_kmod-qca-nss-ppe-vxlanmgr)" = "y" ] ; then \
		$(call scfg, $(1), CONFIG_VXLAN, m); \
		$(call scfg, $(1), CONFIG_OPENVSWITCH_VXLAN, m); \
	fi
	if [ "$(CONFIG_LINUX30)" = "y" ]; then \
		$(call scfg, $(1), CONFIG_BRIDGE_EBT_ARPNAT, n); \
		$(call scfg, $(1), CONFIG_NF_CONNTRACK_EVENTS, y); \
	fi
	if [ "$(JFFS2)" = "y" ]; then \
		if [ "$(CONFIG_LINUX26)" = "y" ]; then \
			$(call scfg, $(1), CONFIG_JFFS2_FS, m); \
			$(call scfg, $(1), CONFIG_JFFS2_FS_DEBUG, 0); \
			$(call scfg, $(1), CONFIG_JFFS2_FS_WRITEBUFFER, y); \
			$(call scfg, $(1), CONFIG_JFFS2_SUMMARY, n); \
			$(call scfg, $(1), CONFIG_JFFS2_FS_XATTR, n); \
			$(call scfg, $(1), CONFIG_JFFS2_COMPRESSION_OPTIONS, y); \
			$(call scfg, $(1), CONFIG_JFFS2_ZLIB, y); \
			$(call scfg, $(1), CONFIG_JFFS2_LZO, n); \
			$(call scfg, $(1), CONFIG_JFFS2_LZMA, n); \
			$(call scfg, $(1), CONFIG_JFFS2_RTIME, n); \
			$(call scfg, $(1), CONFIG_JFFS2_RUBIN, n); \
			$(call scfg, $(1), CONFIG_JFFS2_CMODE_NONE, n); \
			$(call scfg, $(1), CONFIG_JFFS2_CMODE_PRIORITY, y); \
			$(call scfg, $(1), CONFIG_JFFS2_CMODE_SIZE, n); \
		fi; \
		if [ "$(CONFIG_LINUX30)" = "y" ]; then \
			$(call scfg, $(1), CONFIG_JFFS2_FS_WBUF_VERIFY, n); \
			$(call scfg, $(1), CONFIG_JFFS2_CMODE_FAVOURLZO, n); \
		fi; \
	else \
		$(call scfg, $(1), CONFIG_JFFS2_FS, n); \
	fi;
	if [ "$(FTRACE)" = "y" ]; then \
		$(call scfg, $(1), CONFIG_FTRACE, y); \
		$(call scfg, $(1), CONFIG_FUNCTION_TRACER, y); \
		$(call scfg, $(1), CONFIG_FUNCTION_GRAPH_TRACER, y); \
		$(call scfg, $(1), CONFIG_SCHED_TRACER, y); \
		$(call scfg, $(1), CONFIG_FTRACE_SYSCALLS, y); \
		$(call scfg, $(1), CONFIG_BRANCH_PROFILE_NONE, y); \
		$(call scfg, $(1), CONFIG_DYNAMIC_FTRACE, y); \
		$(call scfg, $(1), CONFIG_FUNCTION_PROFILER, y); \
		$(call scfg, $(1), CONFIG_TRACING_EVENTS_GPIO, y); \
	fi;
	if [ "$(UBI)" = "y" ]; then \
		$(call scfg, $(1), CONFIG_MTD_UBI, y); \
		$(call scfg, $(1), CONFIG_MTD_UBI_WL_THRESHOLD, 4096); \
		$(call scfg, $(1), CONFIG_MTD_UBI_BEB_RESERVE, 1); \
		$(call scfg, $(1), CONFIG_MTD_UBI_GLUEBI, y); \
		$(call scfg, $(1), CONFIG_FACTORY_CHECKSUM, y); \
		$(call scfg, $(1), CONFIG_FACTORY_NR_LEB, 4); \
		if [ "$(UBI_DEBUG)" = "y" ]; then \
			$(call scfg, $(1), CONFIG_MTD_UBI_DEBUG, y); \
			$(call scfg, $(1), CONFIG_GCOV_KERNEL, n); \
			$(call scfg, $(1), CONFIG_L2TP_DEBUGFS, n); \
			$(call scfg, $(1), CONFIG_MTD_UBI_DEBUG_MSG, y); \
			$(call scfg, $(1), CONFIG_MTD_UBI_DEBUG_PARANOID, n); \
			$(call scfg, $(1), CONFIG_MTD_UBI_DEBUG_DISABLE_BGT, n); \
			$(call scfg, $(1), CONFIG_MTD_UBI_DEBUG_EMULATE_BITFLIPS, y); \
			$(call scfg, $(1), CONFIG_MTD_UBI_DEBUG_EMULATE_WRITE_FAILURES, y); \
			$(call scfg, $(1), CONFIG_MTD_UBI_DEBUG_EMULATE_ERASE_FAILURES, y); \
			$(call scfg, $(1), CONFIG_MTD_UBI_DEBUG_MSG_BLD, y); \
			$(call scfg, $(1), CONFIG_MTD_UBI_DEBUG_MSG_EBA, y); \
			$(call scfg, $(1), CONFIG_MTD_UBI_DEBUG_MSG_WL, y); \
			$(call scfg, $(1), CONFIG_MTD_UBI_DEBUG_MSG_IO, y); \
			$(call scfg, $(1), CONFIG_JBD_DEBUG, n); \
			$(call scfg, $(1), CONFIG_LKDTM, n); \
			$(call scfg, $(1), CONFIG_DYNAMIC_DEBUG, y); \
			$(call scfg, $(1), CONFIG_SPINLOCK_TEST, n); \
		else \
			$(call scfg, $(1), CONFIG_MTD_UBI_DEBUG, n); \
		fi; \
		if [ "$(UBIFS)" = "y" ]; then \
			$(call scfg, $(1), CONFIG_UBIFS_FS, y); \
			$(call scfg, $(1), CONFIG_UBIFS_FS_XATTR, n); \
			$(call scfg, $(1), CONFIG_UBIFS_FS_ADVANCED_COMPR, y); \
			$(call scfg, $(1), CONFIG_UBIFS_FS_LZO, y); \
			$(call scfg, $(1), CONFIG_UBIFS_FS_ZLIB, y); \
			$(call scfg, $(1), CONFIG_UBIFS_FS_XZ, y); \
			$(call scfg, $(1), CONFIG_UBIFS_FS_DEBUG, n); \
		else \
			$(call scfg, $(1), CONFIG_UBIFS_FS, n); \
		fi; \
		if [ "$(DUMP_OOPS_MSG)" = "y" ]; then \
			$(call scfg, $(1), CONFIG_DUMP_PREV_OOPS_MSG, y); \
			if [ "$(MUSL32)" = "y" ]; then \
				$(call scfg, $(1), CONFIG_DUMP_PREV_OOPS_MSG_BUF_ADDR, 0x4E900000); \
			else \
				$(call scfg, $(1), CONFIG_DUMP_PREV_OOPS_MSG_BUF_ADDR, 0x45300000); \
			fi; \
			$(call scfg, $(1), CONFIG_DUMP_PREV_OOPS_MSG_BUF_LEN, 0x8000); \
		fi; \
		if [ "$(IPV6SUPP)" = "y" ]; then \
			$(call scfg, $(1), CONFIG_IPV6_MULTIPLE_TABLES, y); \
		fi; \
	fi;
	if [ "$(USB)" = "" ]; then \
		$(call scfg, $(1), CONFIG_DIAG_OVER_USB, ); \
		$(call scfg, $(1), CONFIG_EXT2_FS, ); \
		$(call scfg, $(1), CONFIG_EXT3_FS, ); \
		$(call scfg, $(1), CONFIG_EXT4_FS, ); \
		$(call scfg, $(1), CONFIG_JBD2_FS, ); \
		$(call scfg, $(1), CONFIG_FS_MBCACHE, ); \
		$(call scfg, $(1), CONFIG_REISERFS_FS, ); \
		$(call scfg, $(1), CONFIG_XFS_FS, ); \
	fi;
endef

# $1: $(TARGETDIR)
# $2: Top dir of kernel modules. $(TARGETDIR)/lib/modules/x.y.z
define platformGen_Target
endef
