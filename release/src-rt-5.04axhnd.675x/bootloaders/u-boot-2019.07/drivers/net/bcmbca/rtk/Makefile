obj-y += RTK_OBJS.o

MODEL = $(subst -,,$(BUILD_NAME))
RTK_OBJS-objs += RTK_OBJS_preb.o
$(obj)/RTK_OBJS_preb.o:
	cp $(srctree)/drivers/net/bcmbca/rtk/RTK_OBJS.o $(obj)/RTK_OBJS_preb.o
	cp $(srctree)/../obj.$(MODEL)/uboot/drivers/net/bcmbca/rtk/RTK_OBJS.o $(obj)/RTK_OBJS_preb.o
