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

#ifndef MDSS_PANEL_H
#define MDSS_PANEL_H

#include <linux/platform_device.h>
#include <linux/stringify.h>
#include <linux/types.h>
#include <linux/debugfs.h>

/* panel id type */
struct panel_id {
	u16 id;
	u16 type;
};

#define DEFAULT_FRAME_RATE	60
#define DEFAULT_ROTATOR_FRAME_RATE 120
#define MDSS_DSI_RST_SEQ_LEN	10
#define MDSS_MDP_MAX_FETCH 12

/* panel type list */
#define NO_PANEL		0xffff	/* No Panel */
#define EBI2_PANEL		1	/* EBI2 */

/* panel class */
enum {
	DISPLAY_LCD = 0,	/* lcd = ebi2/mddi */
	DISPLAY_LCDC,		/* lcdc */
	DISPLAY_TV,		/* TV Out */
	DISPLAY_EXT_MDDI,	/* External MDDI */
	DISPLAY_WRITEBACK,
};

enum {
	DISPLAY_1 = 0,		/* attached as first device */
	MAX_PHYS_TARGET_NUM,
};

enum {
	MDSS_PANEL_POWER_OFF = 0,
	MDSS_PANEL_POWER_ON,
	MDSS_PANEL_POWER_LP1,
	MDSS_PANEL_POWER_LP2,
};

enum {
	MDSS_PANEL_BLANK_BLANK = 0,
	MDSS_PANEL_BLANK_UNBLANK,
	MDSS_PANEL_BLANK_LOW_POWER,
};

struct lcd_panel_info {
	u32 h_back_porch;
	u32 h_front_porch;
	u32 h_pulse_width;
	u32 v_back_porch;
	u32 v_front_porch;
	u32 v_pulse_width;
	u32 border_clr;
	u32 underflow_clr;
	u32 hsync_skew;
	u32 border_top;
	u32 border_bottom;
	u32 border_left;
	u32 border_right;
	/* Pad width */
	u32 xres_pad;
	/* Pad height */
	u32 yres_pad;
};
struct mdss_panel_info {
	u32 xres;
	u32 yres;
	u32 physical_width;
	u32 physical_height;
	u32 bpp;
	u32 type;
	u32 wait_cycle;
	u32 pdest;
	u32 brightness_max;
	u32 bl_max;
	u32 bl_min;
	u32 fb_num;
	u32 clk_rate;
	u32 clk_min;
	u32 clk_max;
	u32 frame_count;
	u32 is_3d_panel;
	u32 out_format;
	u32 rst_seq[MDSS_DSI_RST_SEQ_LEN];
	u32 rst_seq_len;
	u32 vic; /* video identification code */
	int new_fps;
	int panel_max_fps;
	int panel_max_vtotal;
	u32 mode_gpio_state;
	u32 xstart_pix_align;
	u32 width_pix_align;
	u32 ystart_pix_align;
	u32 height_pix_align;
	u32 min_width;
	u32 min_height;
	u32 min_fps;
	u32 max_fps;

	u32 partial_update_enabled;
	u32 dcs_cmd_by_left;
	int panel_power_state;
	int blank_state;

	uint32_t panel_dead;
	u32 panel_orientation;
	bool dynamic_switch_pending;
	bool is_lpm_mode;

	struct lcd_panel_info lcdc;
};

struct mdss_panel_data {
	struct mdss_panel_info panel_info;
	void (*set_backlight) (struct mdss_panel_data *pdata, u32 bl_level);
	unsigned char *mmss_cc_base;

	/**
	 * event_handler() - callback handler for MDP core events
	 * @pdata:	Pointer refering to the panel struct associated to this
	 *		event. Can be used to retrieve panel info.
	 * @e:		Event being generated, see enum mdss_intf_events
	 * @arg:	Optional argument to pass some info from some events.
	 *
	 * Used to register handler to be used to propagate different events
	 * happening in MDP core driver. Panel driver can listen for any of
	 * these events to perform appropriate actions for panel initialization
	 * and teardown.
	 */
	int (*event_handler) (struct mdss_panel_data *pdata, int e, void *arg);

	struct mdss_panel_data *next;
};

int mdss_register_panel(struct platform_device *pdev,
	struct mdss_panel_data *pdata);

/*
 * mdss_panel_is_power_off: - checks if a panel is off
 * @panel_power_state: enum identifying the power state to be checked
 */
static inline bool mdss_panel_is_power_off(int panel_power_state)
{
	return (panel_power_state == MDSS_PANEL_POWER_OFF);
}

/**
 * mdss_panel_is_power_on_interactive: - checks if a panel is on and interactive
 * @panel_power_state: enum identifying the power state to be checked
 *
 * This function returns true only is the panel is fully interactive and
 * opertaing in normal mode.
 */
static inline bool mdss_panel_is_power_on_interactive(int panel_power_state)
{
	return (panel_power_state == MDSS_PANEL_POWER_ON);
}

/**
 * mdss_panel_is_panel_power_on: - checks if a panel is on
 * @panel_power_state: enum identifying the power state to be checked
 *
 * A panel is considered to be on as long as it can accept any commands
 * or data. Sometimes it is posible to program the panel to be in a low
 * power non-interactive state. This function returns false only if panel
 * has explicitly been turned off.
 */
static inline bool mdss_panel_is_power_on(int panel_power_state)
{
	return !mdss_panel_is_power_off(panel_power_state);
}

/**
 * mdss_panel_is_panel_power_on_lp: - checks if a panel is in a low power mode
 * @pdata: pointer to the panel struct associated to the panel
 * @panel_power_state: enum identifying the power state to be checked
 *
 * This function returns true if the panel is in an intermediate low power
 * state where it is still on but not fully interactive. It may or may not
 * accept any commands and display updates.
 */
static inline bool mdss_panel_is_power_on_lp(int panel_power_state)
{
	return !mdss_panel_is_power_off(panel_power_state) &&
		!mdss_panel_is_power_on_interactive(panel_power_state);
}

/**
 * mdss_panel_is_panel_power_on_ulp: - checks if panel is in ultra low power mode
 * @pdata: pointer to the panel struct associated to the panel
 * @panel_power_state: enum identifying the power state to be checked
 *
 * This function returns true if the panel is in a ultra low power
 * state where it is still on but cannot recieve any display updates.
 */
static inline bool mdss_panel_is_power_on_ulp(int panel_power_state)
{
	return panel_power_state == MDSS_PANEL_POWER_LP2;
}

#endif /* MDSS_PANEL_H */
