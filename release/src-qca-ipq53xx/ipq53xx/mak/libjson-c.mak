libjson-c:
	$(MAKE) -C $@ && $(MAKE) $@-stage

libjson-c-clean:
	$(MAKE) -C libjson-c clean

libjson-c-stage:
	$(MAKE) -C libjson-c stage
