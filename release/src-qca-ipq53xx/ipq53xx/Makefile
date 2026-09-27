#
# Platform specific router Makefile
#
# Copyright 2016, Broadcom
# All Rights Reserved.
#
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
#

include ../router/common.mak
include $(SRCBASE)/.config
include qsdk.config

export CFLAGS+=$(EXTRACFLAGS)
export LDFLAGS+=$(EXTRALDFLAGS)

# Package dependence, sync with ipq53xx/upb-platform_router.mak
PKGDEP_464XLAT		:= nat46
PKGDEP_ATHTESTCMD_LITH	:= libnl libnl-tiny-0.1 libtcmd
PKGDEP_BREAKPAD_WRAPPER	:= breakpad
PKGDEP_EMESH_SP		:=
PKGDEP_FTM		:= libnl libtcmd qca-diag libprovision libjson-c common-headers
PKGDEP_IW		:= libnl-tiny-0.1
PKGDEP_LIBATHTESTCMD	:= libnl libtcmd libtlv2 libtlvutil
PKGDEP_LIBIEEE1905	:= libqcacommon
PKGDEP_LIBLUA		:=
PKGDEP_LIBROXML		:=
PKGDEP_LIBSTORAGE	:= qca-wifi libqcacommon libuci libwifisoncfg
PKGDEP_LIBTCMD		:= libnl
PKGDEP_LIBTLV2		:= libnl
PKGDEP_LIBTLVUTIL	:= libnl
PKGDEP_LIBUBOX		:=
PKGDEP_LIBUBUS		:= libubox
PKGDEP_LIBUCI		:= libubox
PKGDEP_LIBWIFISONCFG	:= qca-wifi qca-cfg80211 libqcacommon libnl
PKGDEP_LIBWPA2		:=
PKGDEP_MCPROXY		:=
PKGDEP_MINIDUMP		:=
PKGDEP_MYFTM		:= libtcmd libnl libathtestcmd
PKGDEP_QCA_CFG80211	:= libxml2 libnl iw libnl-tiny-0.1
PKGDEP_QCA_CFG80211TOOL	:= libnl libroxml qca-wifi
PKGDEP_QCA_CNSS		:=
PKGDEP_QCA_CNSS_DAEMON	:= libnl qca-qmi-framework breakpad breakpad-wrapper
PKGDEP_QCA_CNSSDIAG	:= libnl qca-diag common-headers
PKGDEP_QCA_DIAG		:= common-headers
PKGDEP_QCA_EZMESH	:= qcawifi qca-ssdk libqcacommon qca-cfg80211 qca-hyfi-bridge \
			   qca-libhyfi-bridge libwifisoncfg libubox libuci libwpa2 libubus
PKGDEP_QCA_HOSTAP	:= qca-wifi libnl openssl qca-nss-macsec breakpad-wrapper
PKGDEP_QCA_HYCTL	:= qca-hyfi-bridge qca-libhyfi-bridge
PKGDEP_QCA_HYFI_BRIDGE	:= qca-ssdk emesh-sp
PKGDEP_QCA_LIBHYFI_BRIDGE := qca-hyfi-bridge
PKGDEP_QCA_MAD		:= libieee1905 libnl qca-cfg80211 libqcacommon jansson-2.7
PKGDEP_QCA_MCS_APPS	:= qca-mcs-lkm qca-cfg80211 qca-ssdk qca-wifi libqcacommon
PKGDEP_QCA_MCS_LKM	:= $(if $(CONFIG_PACKAGE_kmod-qca-ovsmgr),qca-ovsmgr)
PKGDEP_QCA_NSM_APP	:= qca-nsm-lib
PKGDEP_QCA_NSM_LIB	:= libnl qca-nss-nsm
PKGDEP_QCA_NSS_DP	:= qca-ssdk
PKGDEP_QCA_NSS_ECM	:= nat46 qca-mcs-lkm qca-hyfi-bridge emesh-sp qca-ovsmgr qca-nss-sfe
PKGDEP_QCA_NSS_EIP	:= qca-nss-ppe qca-nss-ecm
PKGDEP_QCA_NSS_FLS	:= qca-nss-sfe
PKGDEP_QCA_NSS_MACSEC	:=
PKGDEP_QCA_NSS_NSM	:= qca-nss-sfe qca-nss-ppe qca-nss-dp
PKGDEP_QCA_NSS_PPE	:= qca-ssdk nat46 qca-ovsmgr
PKGDEP_QCA_NSS_PPE_CLIENTS	:= qca-nss-ppe qca-nss-ppe-vlan-mgr
PKGDEP_QCA_NSS_PPE_DS	:= qca-nss-ppe-vp qca-nss-dp
PKGDEP_QCA_NSS_PPE_GRETAP := qca-nss-ppe-tun
PKGDEP_QCA_NSS_PPE_MAPT := qca-nss-ppe-tun nat46
PKGDEP_QCA_NSS_PPE_RULE	:= qca-nss-ppe qca-nss-ppe-vp
PKGDEP_QCA_NSS_PPE_TUN	:= qca-nss-ppe qca-nss-ppe-vp
PKGDEP_QCA_NSS_PPE_TUNIPIP6 := qca-nss-ppe-tun
PKGDEP_QCA_NSS_PPE_VP	:= qca-nss-ppe qca-nss-dp
PKGDEP_QCA_NSS_PPE_VXLANMGR := qca-nss-ppe-tun
PKGDEP_QCA_NSS_SFE	:= qca-nss-ppe
PKGDEP_QCA_OVSMGR	:=
PKGDEP_QCA_SAL_QOS_TEST	:= json-c qca-sal-qos
PKGDEP_QCA_SAL_RULE_TEST:= json-c libnl qca-sal-rule
PKGDEP_QCA_WHC_LBD	:= qca-wifi libqcacommon jansson-2.7 libnl libwifisoncfg qca-wifison-ext-lib libieee1905
PKGDEP_QCA_WHC_REPACD	:= qca-wifi libnl qca-cfg80211 libieee1905 qca-ssdk-shell qca-wsplcd
PKGDEP_QCA_WIFI_FW	:=
PKGDEP_QCA_WIFI		:= iproute2-5.18.0 libxml2 libnl qca-cfg80211 libroxml qca-sal-rule qca-nss-ppe qca-nss-ppe-ds
PKGDEP_QCA_WIFISON_EXT_LIB := libnl qca-whc-repacd
PKGDEP_QCA_WSPLCD	:= qca-wifi libqcacommon libwpa2 libieee1905 libwifisoncfg

#
# software package selection
#
pobj-y:=

# IPQ5322.ILQ.12.2 CSU1
pobj-y += 464xlat $(PKGDEP_464XLAT)
pobj-y += athtestcmd-lith $(PKGDEP_ATHTESTCMD_LITH)
pobj-y += breakpad
pobj-y += breakpad-wrapper $(PKGDEP_BREAKPAD_WRAPPER)
pobj-n += ctlapp_bin
pobj-y += emesh-sp $(PKGDEP_EMESH_SP)
pobj-y += ftm $(PKGDEP_FTM)
pobj-y += libathtestcmd $(PKGDEP_LIBATHTESTCMD)
pobj-y += liblua $(PKGDEP_LIBLUA)
pobj-y += libnl-tiny-0.1
pobj-y += libstorage $(PKGDEP_LIBSTORAGE)
pobj-y += libtlv2 $(PKGDEP_LIBTLV2)
pobj-y += libtlvutil $(PKGDEP_LIBTLVUTIL)
pobj-y += libubox $(PKGDEP_LIBUBOX)
pobj-y += libuci $(PKGDEP_LIBUCI)
pobj-y += libwifisoncfg $(PKGDEP_LIBWIFISONCFG)
pobj-y += libwpa2 $(PKGDEP_LIBWPA2)
pobj-y += mcproxy $(PKGDEP_MCPROXY)
pobj-y += minidump $(PKGDEP_MINIDUMP)
pobj-y += myftm $(PKGDEP_MYFTM)
pobj-y += qca-cfg80211 $(PKGDEP_QCA_CFG80211)
pobj-y += qca-cfg80211tool $(PKGDEP_QCA_CFG80211TOOL)
pobj-y += qca-cnss
pobj-y += qca-cnss-daemon $(PKGDEP_QCA_CNSS_DAEMON)
pobj-y += qca-cnssdiag $(PKGDEP_QCA_CNSSDIAG)
pobj-y += qca-diag $(PKGDEP_QCA_DIAG)
pobj-y += qca-ezmesh $(PKGDEP_QCA_EZMESH)
pobj-y += qca-hostap $(PKGDEP_QCA_HOSTAP)
pobj-y += qca-hyctl $(PKGDEP_QCA_HYCTL)
pobj-y += qca-mad $(PKGDEP_QCA_MAD)
pobj-y += qca-mcs-apps $(PKGDEP_QCA_MCS_APPS)
pobj-y += qca-mcs-lkm $(PKGDEP_QCA_MCS_LKM)
pobj-y += qca-nsm-app $(PKGDEP_QCA_NSM_APP)
pobj-y += qca-nsm-lib $(PKGDEP_QCA_NSM_LIB)
pobj-y += qca-nss-dp $(PKGDEP_QCA_NSS_DP)
pobj-y += qca-nss-ecm $(PKGDEP_QCA_NSS_ECM)
pobj-y += qca-nss-eip $(PKGDEP_QCA_NSS_EIP)
pobj-y += qca-nss-fls $(PKGDEP_QCA_NSS_FLS)
pobj-y += qca-nss-macsec $(PKGDEP_QCA_NSS_MACSEC)
pobj-y += qca-nss-nsm $(PKGDEP_QCA_NSS_NSM)
pobj-y += qca-nss-ppe $(PKGDEP_QCA_NSS_PPE)
pobj-y += qca-nss-ppe-clients $(PKGDEP_QCA_NSS_PPE_CLIENTS)
pobj-y += qca-nss-ppe-ds $(PKGDEP_QCA_NSS_PPE_DS)
pobj-y += qca-nss-ppe-gretap $(PKGDEP_QCA_NSS_PPE_GRETAP)
pobj-y += qca-nss-ppe-mapt $(PKGDEP_QCA_NSS_PPE_MAPT)
pobj-y += qca-nss-ppe-rule $(PKGDEP_QCA_NSS_PPE_RULE)
pobj-y += qca-nss-ppe-tun $(PKGDEP_QCA_NSS_PPE_TUN)
pobj-y += qca-nss-ppe-tunipip6 $(PKGDEP_QCA_NSS_PPE_TUNIPIP6)
pobj-y += qca-nss-ppe-vp $(PKGDEP_QCA_NSS_PPE_VP)
pobj-$(CONFIG_PACKAGE_kmod-qca-nss-ppe-vxlanmgr) += qca-nss-ppe-vxlanmgr $(PKGDEP_QCA_NSS_PPE_VXLANMGR)
pobj-y += qca-nss-sfe
pobj-y += qca-ovsmgr $(PKGDEP_QCA_OVSMGR)
pobj-y += qca-qmi-framework
pobj-y += qca-sal-qos
pobj-y += qca-sal-rule
pobj-y += qca-ssdk
pobj-y += qca-ssdk-shell
pobj-y += qca-thermald
pobj-y += qca-whc-lbd $(PKGDEP_QCA_WHC_LBD)
pobj-y += qca-whc-repacd $(PKGDEP_QCA_WHC_REPACD)
pobj-y += qca-wifi $(PKGDEP_QCA_WIFI)
pobj-y += qca-wifi-fw $(PKGDEP_QCA_WIFI_FW)
pobj-y += qca-wifison-ext-lib $(PKGDEP_QCA_WIFISON_EXT_LIB)
pobj-y += qca-wsplcd $(PKGDEP_QCA_WSPLCD)

# Wi-Fi SON related
pobj-y += qca-hyfi-bridge $(PKGDEP_QCA_HYFI_BRIDGE)
pobj-y += qca-libhyfi-bridge $(PKGDEP_QCA_LIBHYFI_BRIDGE)

# U-Boot binaries
pobj-y += uboot_bins

pobj-clean := $(foreach pobj, $(pobj-y) $(pobj-n) $(pobj-), $(pobj)-clean)
pobj-install := $(foreach pobj,$(pobj-y),$(pobj)-install)

#
# Basic rules
#

all: qsdk.config $(pobj-y)

install: $(pobj-install) gen_target

gen_target:
	[ -d $(TARGETDIR) ] || install -d $(TARGETDIR)
	for dir in $(wildcard $(patsubst %,$(INSTALLDIR)/%,$(pobj-y))) ; do \
		(cd $${dir} && tar cpf - .) | (cd $(TARGETDIR) && tar xpf -) \
	done

clean: $(pobj-clean)

distclean: clean

#
# include rules for platform specific software packages
#
-include $(wildcard mak/*.mak)

#
# configuration
#

#
# overrides and extra dependencies
#

#
# Generic rules
#

%: qsdk.config
	@[ ! -d $* ] || [ -f $*/Makefile ] || $(MAKE) $*-configure
	@[ ! -d $* ] || ( $(MAKE) -C $* )


%-clean:
	-@[ ! -d $* ] || $(MAKE) -C $* clean


%-install: %
	@echo $*
	@[ ! -d $* ] || $(MAKE) -C $* install INSTALLDIR=$(INSTALLDIR)/$*

%-stage:
	@echo $*
	@[ ! -d $* ] || $(MAKE) -C $* install DESTDIR=$(STAGEDIR) INSTALLDIR=$(INSTALLDIR)/$*

%-build:
	$(MAKE) $*-clean $*

%/Makefile:
	[ ! -d $* ] || $(MAKE) $*-configure

%-configure:
	@[ ! -d $* ] || ( cd $* ; \
		$(CONFIGURE) \
		--prefix=/usr \
		--bindir=/usr/sbin \
		--libdir=/usr/lib \
	)

qsdk.config:
ifeq ($(MUSL64),y)
	ln -sf musl_qsdk64.config qsdk.config
else ifeq ($(MUSL32),y)
	ln -sf musl_qsdk32.config qsdk.config
else
	ln -sf qsdk32.config qsdk.config
endif

$(pobj-y) $(pobj-n) $(pobj-clean) $(pobj-install): dummy

.PHONY: all clean distclean mrproper install package image
.PHONY: conf mconf oldconf kconf kmconf config menuconfig oldconfig
.PHONY: dummy libnet libpcap


