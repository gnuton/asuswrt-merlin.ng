libnl-tiny-0.1:
	$(MAKE) -C $@ && $(MAKE) $@-stage

libnl-tiny-0.1-clean:
	$(MAKE) -C libnl-tiny-0.1 clean

libnl-tiny-0.1-stage:
	$(MAKE) -C libnl-tiny-0.1 stage
