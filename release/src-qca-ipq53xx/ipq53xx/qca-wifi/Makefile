INSTALLKMODDIR:=$(INSTALLDIR)/lib/modules/$(LINUX_KERNEL)

all:

install:
	@mkdir -p $(addprefix $(INSTALLDIR)/,usr/sbin usr/lib lib/wifi/debug ini \
		data/vendor sawf) $(INSTALLKMODDIR)
	@[ ! -d $(INSTALLDIR)/data/vendor/wifi ] || rmdir $(INSTALLDIR)/data/vendor/wifi
	@ln -sf /tmp/wifi $(INSTALLDIR)/data/vendor/wifi
	@[ -d $(INSTALLDIR)/bin ] || install -d $(INSTALLDIR)/bin
	@ln -sf /usr/sbin/cfg80211tool $(INSTALLDIR)/bin/iwpriv
	@install $(foreach f,prebuilt/lib/wifi/*,$(f)) $(INSTALLDIR)/lib/wifi
	@ln -sf /usr/sbin/cfg80211tool $(INSTALLDIR)/usr/sbin/cfg80211tool_mesh
	@cd prebuilt/ini && tar cvf - . | tar xvf - -C $(INSTALLDIR)/ini
	@cd prebuilt/sawf && tar cvf - . | tar xvf - -C $(INSTALLDIR)/sawf
	@install $(foreach f,prebuilt/usr/sbin/*,$(f)) $(INSTALLDIR)/usr/sbin
	@-$(STRIP) $(INSTALLDIR)/usr/sbin/*
	@install $(foreach f,prebuilt/usr/lib/*,$(f)) $(INSTALLDIR)/usr/lib
	@$(STRIP) $(INSTALLDIR)/usr/lib/*
	@install $(foreach f,prebuilt/modules/*,$(f)) $(INSTALLKMODDIR)
	@$(STRIPX) $(INSTALLKMODDIR)/*.ko

clean:

stage:
