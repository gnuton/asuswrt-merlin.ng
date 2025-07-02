cd obj-$(CONFIG_BCM_CLEDS_PROC_DRIVER) += bcm_cleds.o

bcm_cleds-objs += bcm_cleds_dep.o

$(obj)/bcm_cleds_dep.o:
	cp $(obj)/../../../../../router-sysdep.$(shell echo $(BUILD_NAME) | tr A-Z a-z)/hnd_extra/prebuilt/bcm_cleds.o $(obj)/bcm_cleds_dep.o
