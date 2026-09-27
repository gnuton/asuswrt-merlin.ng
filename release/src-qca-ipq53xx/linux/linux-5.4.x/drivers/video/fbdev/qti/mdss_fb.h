/* Copyright (c) 2008-2014, 2020 The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef MDSS_FB_H
#define MDSS_FB_H

#include <linux/fb.h>
#include <linux/types.h>

#include "mdss_panel.h"

#define MSM_FB_DEFAULT_PAGE_SIZE 2
#define MFD_KEY  0x11161126
#define MSM_FB_MAX_DEV_LIST 32

struct disp_info_type_suspend {
	int op_enable;
	int panel_power_state;
};
struct msm_fb_data_type;

struct msm_mdp_interface {
	int (*fb_mem_alloc_fnc)(struct msm_fb_data_type *mfd);
	int (*init_fnc)(struct msm_fb_data_type *mfd);
	int (*on_fnc)(struct msm_fb_data_type *mfd);
	int (*off_fnc)(struct msm_fb_data_type *mfd);
	/* called to release resources associated to the process */
	int (*release_fnc)(struct msm_fb_data_type *mfd, bool release_all);
	int (*ioctl_handler)(struct msm_fb_data_type *mfd, u32 cmd, void *arg);
	void (*dma_fnc)(struct msm_fb_data_type *mfd);
	int (*cursor_update)(struct msm_fb_data_type *mfd,
				struct fb_cursor *cursor);
	int (*panel_register_done)(struct mdss_panel_data *pdata);
	u32 (*fb_stride)(u32 fb_index, u32 xres, int bpp);
	int (*configure_panel)(struct msm_fb_data_type *mfd, int mode);
	void *private1;
};

struct msm_fb_data_type {
	u32 key;
	u32 index;
	u32 ref_cnt;
	u32 fb_page;

	struct panel_id panel;
	struct mdss_panel_info *panel_info;

	u32 dest;
	struct fb_info *fbi;

	int op_enable;
	u32 fb_imgType;
	int panel_reconfig;

	u32 dst_format;
	int panel_power_state;
	struct disp_info_type_suspend suspend;

	struct platform_device *pdev;

	struct msm_mdp_interface mdp;
};

static inline bool mdss_fb_is_power_off(struct msm_fb_data_type *mfd)
{
	return mdss_panel_is_power_off(mfd->panel_power_state);
}

static inline bool mdss_fb_is_power_on_interactive(
	struct msm_fb_data_type *mfd)
{
	return mdss_panel_is_power_on_interactive(mfd->panel_power_state);
}

static inline bool mdss_fb_is_power_on(struct msm_fb_data_type *mfd)
{
	return mdss_panel_is_power_on(mfd->panel_power_state);
}

static inline bool mdss_fb_is_power_on_lp(struct msm_fb_data_type *mfd)
{
	return mdss_panel_is_power_on_lp(mfd->panel_power_state);
}

int mdss_fb_get_phys_info(dma_addr_t *start, unsigned long *len, int fb_num);
int mdss_fb_register_mdp_instance(struct msm_mdp_interface *mdp);
int mdss_fb_suspres_panel(struct device *dev, void *data);
int mdss_fb_do_ioctl(struct fb_info *info, unsigned int cmd,
		     unsigned long arg);
int mdss_fb_compat_ioctl(struct fb_info *info, unsigned int cmd,
			 unsigned long arg);
#endif /* MDSS_FB_H */
