ccflags-y += -I$(src)
ccflags-y += -I$(srctree)/arch/arm/mach-qcom/include
ccflags-y += -I$(srctree)/arch/arm/mach-qcom

mdss-qpic-objs := mdss_qpic.o mdss_fb.o mdss_qpic_panel.o
obj-$(CONFIG_FB_QTI_QPIC) += mdss-qpic.o
obj-$(CONFIG_FB_QTI_QPIC_ER_SSD1963_PANEL) += qpic_panel_er_ssd1963.o
