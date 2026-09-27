openssl:
	$(MAKE) -C $(TOP) obj-y=openssl $@

openssl-install:
	$(MAKE) -C $(TOP) obj-y=openssl $@

openssl-clean:
	$(MAKE) -C $(TOP) obj-y=openssl $@
