ifeq ($(MUSL32),y)
export GCC_CPU_PARAM := -march=armv7-a -mcpu=cortex-a8
else ifeq ($(MUSL64),y)
export GCC_CPU_PARAM := -march=armv8-a -mcpu=cortex-a53+crypto
endif
EXTRACFLAGS := -DLINUX26 -DCONFIG_QCA -DDEBUG_NOISY -DDEBUG_RCTEST -DMUSL_LIBC -fPIC -pipe -funit-at-a-time -Wno-pointer-sign $(GCC_CPU_PARAM) \
	       -fno-caller-saves -fno-plt -Wa,--noexecstack -fhonour-copts -fstack-protector-strong -D_FORTIFY_SOURCE=2 -Wl,-z,now -Wl,-z,relro

ifneq ($(findstring linux-3,$(LINUXDIR)),)
EXTRACFLAGS += -DLINUX30
endif

export EXTRACFLAGS
