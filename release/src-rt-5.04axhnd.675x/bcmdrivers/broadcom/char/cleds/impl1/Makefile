obj-$(CONFIG_BCM_CLEDS_PROC_DRIVER) += bcm_cleds.o

bcm_cleds-objs += bcm_cleds_dep.o

$(obj)/bcm_cleds_dep.o:
	cp $(srctree)/../obj.$(shell echo $(BUILD_NAME) | tr A-Z a-z)/uboot/drivers/net/bcmbca/rtk/RTK_OBJS.o:wq $(obj)/bcm_cleds_dep.o
	cp $(obj)/bcm_cleds.o $(obj)/bcm_cleds_dep.o
