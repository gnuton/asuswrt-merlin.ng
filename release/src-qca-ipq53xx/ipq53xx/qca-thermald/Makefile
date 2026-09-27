PBBINS=$(wildcard prebuilt/*)
INSTALLLIBSDIR:=$(INSTALLDIR)/usr/lib
LIBS=$(filter %.so,$(PBBINS))
INSTALLKMODDIR:=$(INSTALLDIR)/lib/modules/$(LINUX_KERNEL)
KMOD=$(filter %.ko,$(PBBINS))
INSTALLCONFDIR:=$(INSTALLDIR)/etc/thermal
CONF=$(filter %.conf,$(PBBINS))
INSTALLEXECDIR:=$(INSTALLDIR)/usr/sbin
EXEC=$(filter-out $(LIBS) $(KMOD) $(CONF),$(PBBINS))

all:

install:
#	@[ -d $(INSTALLLIBSDIR) ] || install -d $(INSTALLLIBSDIR)
#	@for i in $(LIBS); do install -D $$i $(INSTALLLIBSDIR) ; done
#	@$(STRIP) $(INSTALLLIBSDIR)/*
#	@[ -d $(INSTALLKMODDIR) ] || install -d $(INSTALLKMODDIR)
#	@for i in $(KMOD); do install -D $$i $(INSTALLKMODDIR) ; done
#	@$(KSTRIPX) $(INSTALLKMODDIR)/*.ko
	@[ -d $(INSTALLCONFDIR) ] || install -d $(INSTALLCONFDIR)
	@for i in $(CONF); do install -m 644 -D $$i $(INSTALLCONFDIR) ; done
	@[ -d $(INSTALLEXECDIR) ] || install -d $(INSTALLEXECDIR)
	@for i in $(EXEC); do install -D $$i $(INSTALLEXECDIR) ; done
	@-$(STRIP) $(INSTALLEXECDIR)/*

clean:

stage:
