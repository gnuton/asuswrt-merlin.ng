obj-m += rtl8372.o
rtl8372-objs += rtl8372_preb.o

ifeq ($(MODEL),GTBE98)
# Never derive the input from rtl8372.o: it is also the composite output and
# can contain another model's staged object after a previous build.
gtbe98-prebuilt := $(abspath $(srctree)/../../router-sysdep.gt-be98/hnd_extra/prebuilt/rtl8372.o)
gtbe98-prebuilt-sha256 := 3e1948756cec3f05e217c637dc9debe7a9addfc93749feb3653fb9a3006e38f7
rtl8372-objs += gtbe98_readback_adapter.o
obj-m += gtbe98_rtl8372_readback.o

$(obj)/rtl8372_preb.o: $(gtbe98-prebuilt) FORCE
	@printf '%s  %s\n' '$(gtbe98-prebuilt-sha256)' '$(gtbe98-prebuilt)' | sha256sum --check --status || { echo 'GT-BE98 RTL8372 readback: unsupported prebuilt SHA256' >&2; exit 1; }
# platform.mak can overwrite rtl8372.o with a fresh prebuilt. Refresh this
# prerequisite each pass so Kbuild always relinks the adapter into the result.
	$(Q)cp '$(gtbe98-prebuilt)' '$@'
else
$(obj)/rtl8372_preb.o:
	cp $(obj)/rtl8372.o $(obj)/rtl8372_preb.o
endif
