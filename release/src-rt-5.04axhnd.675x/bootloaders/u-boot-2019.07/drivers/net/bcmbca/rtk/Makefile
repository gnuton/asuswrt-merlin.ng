obj-y += RTK_OBJS.o

BUILD_NAME_LC := $(shell echo $(BUILD_NAME) | tr '[:upper:]' '[:lower:]')

RTK_OBJS-objs += RTK_OBJS_preb.o
$(obj)/RTK_OBJS_preb.o:
	cp $(srctree)/../obj.$(BUILD_NAME_LC)/uboot/drivers/net/bcmbca/rtk/RTK_OBJS.o $(obj)/RTK_OBJS_preb.o
