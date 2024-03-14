obj-y += RTK_OBJS.o

# MODEL_B = $(subst -,,$(BUILD_NAME))
# MODEL_LC = $(shell echo $(MODEL_B) | tr '[:upper:]' '[:lower:]')
MODEL_E = $(subst -,,$(MODEL))
RTK_OBJS-objs += RTK_OBJS_preb.o
$(obj)/RTK_OBJS_preb.o:
	cp $(srctree)/../obj.$(MODEL_E)/uboot/drivers/net/bcmbca/rtk/RTK_OBJS.o $(obj)/RTK_OBJS_preb.o
