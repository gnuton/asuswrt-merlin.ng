PBBINS=$(wildcard prebuilt/*)
INSTALLLIBSDIR:=$(INSTALLDIR)/usr/lib

all:

install:
	@[ -d $(INSTALLLIBSDIR) ] || install -d $(INSTALLLIBSDIR)
	@for i in $(PBBINS); do cp -af $$i $(INSTALLLIBSDIR) ; done
	@$(STRIP) $(INSTALLLIBSDIR)/*

clean:

stage:
