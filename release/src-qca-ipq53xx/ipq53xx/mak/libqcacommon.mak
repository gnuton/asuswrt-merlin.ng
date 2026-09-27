libqcacommon:
	[ ! -e $@ ] || $(MAKE) -C $@ && $(MAKE) $@-stage

libqcacommon-install: libqcacommon
	[ ! -e libqcacommon ] || $(MAKE) -C $< INSTALLDIR=$(INSTALLDIR)/$< install

libqcacommon-clean:
	[ ! -e libqcacommon ] || $(MAKE) -C libqcacommon clean

libqcacommon-stage:
	[ ! -e libqcacommon ] || $(MAKE) -C libqcacommon stage

