obj-y += RTK_OBJS.o

# HACK: BUILD_NAME and MODEL are lacks of '-'. 
# Check if BUILD_NAME contains RTAX58U_V2
ifeq ($(findstring RTAX58U_V2,$(BUILD_NAME)),RTAX58U_V2)
    MODEL = rt-ax58u_v2
else
    # Otherwise, exit with an error message
    $(error Build name not defined)
endif

RTK_OBJS-objs += RTK_OBJS_preb.o
$(obj)/RTK_OBJS_preb.o:
	cp $(srctree)/../obj.$(MODEL)/uboot/drivers/net/bcmbca/rtk/RTK_OBJS.o $(obj)/RTK_OBJS_preb.o
