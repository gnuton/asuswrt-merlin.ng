PBBINS=$(wildcard prebuilt/*)
INSTALLLIBSDIR:=$(INSTALLDIR)/usr/lib
LIBS=$(filter %.so,$(PBBINS))
INSTALLKMODDIR:=$(INSTALLDIR)/lib/modules/$(LINUX_KERNEL)
KMOD=$(filter %.ko,$(PBBINS))
INSTALLEXECDIR:=$(INSTALLDIR)/usr/bin
SH=$(filter %.sh,$(PBBINS))
EXEC=$(filter-out $(LIBS) $(KMOD) $(SH),$(PBBINS))

all:

install:
#	@[ -d $(INSTALLLIBSDIR) ] || install -d $(INSTALLLIBSDIR)
#	@for i in $(LIBS); do install -D $$i $(INSTALLLIBSDIR) ; done
#	@$(STRIP) $(INSTALLLIBSDIR)/*
#	@[ -d $(INSTALLKMODDIR) ] || install -d $(INSTALLKMODDIR)
#	@for i in $(KMOD); do install -D $$i $(INSTALLKMODDIR) ; done
#	@$(KSTRIPX) $(INSTALLKMODDIR)/*.ko
	@[ -d $(INSTALLEXECDIR) ] || install -d $(INSTALLEXECDIR)
	@for i in $(EXEC); do install -D $$i $(INSTALLEXECDIR) ; done
	@-$(STRIP) $(INSTALLEXECDIR)/*
	@for i in $(SH); do install -D $$i $(INSTALLDIR)/usr/sbin ; done

clean:

stage:
