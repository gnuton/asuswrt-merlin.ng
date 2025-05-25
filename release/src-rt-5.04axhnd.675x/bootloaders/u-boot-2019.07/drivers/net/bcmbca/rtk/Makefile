obj-y += RTK_OBJS.o

RTK_OBJS-objs += RTK_OBJS_preb.o
$(obj)/RTK_OBJS_preb.o:
	cp $(srctree)/../obj.$(shell echo $(BUILD_NAME) | tr A-Z a-z)/uboot/drivers/net/bcmbca/rtk/RTK_OBJS.o $(obj)/RTK_OBJS_preb.o
