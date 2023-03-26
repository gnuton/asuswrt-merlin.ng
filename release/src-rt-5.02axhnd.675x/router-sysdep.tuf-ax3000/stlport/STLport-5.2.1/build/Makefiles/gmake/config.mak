COMPILER_NAME := gcc
TARGET_OS := arm-buildroot-linux-gnueabi
BASE_INSTALL_DIR := ${DESTDIR}/home/gitserv_asus/GPL/ASUS_GPL/3000/asuswrt/release/src-rt-5.02axhnd.675x/targets/96750GW/fs.build/public
EXTRA_CXXFLAGS := -std=gnu++98
EXTRA_CFLAGS := -DSUPPORT_DDR_SELF_REFRESH_PWRSAVE -DSUPPORT_ETH_PWRSAVE -DSUPPORT_ENERGY_EFFICIENT_ETHERNET -DSUPPORT_ETH_DEEP_GREEN_MODE -DLINUX -DCHIP_63178 -DCONFIG_BCM963178 -Os -march=armv7-a -fomit-frame-pointer -mno-thumb-interwork -mabi=aapcs-linux -marm -fno-common -ffixed-r8 -msoft-float -D__ARM_ARCH_7A__ -Wno-date-time -Wall -Darm -g -fPIC -I/opt/toolchains//crosstools-arm-gcc-5.5-linux-4.1-glibc-2.26-binutils-2.28.1/usr/include -L/opt/toolchains//crosstools-arm-gcc-5.5-linux-4.1-glibc-2.26-binutils-2.28.1/usr/lib -Wno-date-time -DCONFIG_BCM_MAX_GEM_PORTS=1 -I/home/gitserv_asus/GPL/ASUS_GPL/3000/asuswrt/release/src-rt-5.02axhnd.675x/userspace/private/libs/wlcsm/include -DBRCM_WLAN -DWIRELESS -DDSLCPE -DBCA_CPEROUTER -DBCA_HNDROUTER
