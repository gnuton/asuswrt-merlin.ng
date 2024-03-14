obj-y += RTK_OBJS.o

MODEL = $(subst -,,$(BUILD_NAME))
MODEL_LC = $(shell echo $(MODEL) | tr '[:upper:]' '[:lower:]')

RTK_OBJS-objs += RTK_OBJS_preb.o
$(obj)/RTK_OBJS_preb.o:
	cp $(srctree)/../obj.$(MODEL_LC)/uboot/drivers/net/bcmbca/rtk/RTK_OBJS.o $(obj)/RTK_OBJS_preb.o
