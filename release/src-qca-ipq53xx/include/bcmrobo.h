/*
 * RoboSwitch setup functions
 *
 * Copyright (C) 2009, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: bcmrobo.h,v 13.7.2.2 2009/07/17 03:11:38 Exp $
 */

#ifndef _bcm_robo_h_
#define _bcm_robo_h_

#define	DEVID5325	0x25	/* 5325 (Not really but we fake it) */
#define	DEVID5395	0x95	/* 5395 */
#define	DEVID5397	0x97	/* 5397 */
#define	DEVID5398	0x98	/* 5398 */
#define	DEVID53115	0x3115	/* 53115 */

/* Power save duty cycle times */
#define MAX_NO_PHYS		5
#define PWRSAVE_SLEEP_TIME	12
#define PWRSAVE_WAKE_TIME	3

/* Power save modes for the switch */
#define ROBO_PWRSAVE_NORMAL 		0
#define ROBO_PWRSAVE_AUTO		1
#define ROBO_PWRSAVE_MANUAL		2
#define ROBO_PWRSAVE_AUTO_MANUAL 	3

/* Forward declaration */
typedef struct robo_info_s robo_info_t;

/* Device access/config oprands */
typedef struct {
	/* low level routines */
	void (*enable_mgmtif)(robo_info_t *robo);	/* enable mgmt i/f, optional */
	void (*disable_mgmtif)(robo_info_t *robo);	/* disable mgmt i/f, optional */
	int (*write_reg)(robo_info_t *robo, uint8 page, uint8 reg, void *val, int len);
	int (*read_reg)(robo_info_t *robo, uint8 page, uint8 reg, void *val, int len);
	/* description */
	char *desc;
} dev_ops_t;


typedef	uint16 (*miird_f)(void *h, int add, int off);
typedef	void (*miiwr_f)(void *h, int add, int off, uint16 val);

/* Private state per RoboSwitch */
struct robo_info_s {
	si_t	*sih;			/* SiliconBackplane handle */
	char	*vars;			/* nvram variables handle */
	void	*h;			/* dev handle */
	uint16	devid;			/* Device id for the switch */
	uint32	corerev;		/* Core rev of internal switch */

	dev_ops_t *ops;			/* device ops */
	uint8	page;			/* current page */

	/* SPI */
	uint32	ss, sck, mosi, miso;	/* GPIO mapping */

	/* MII */
	miird_f	miird;
	miiwr_f	miiwr;

	uint16	prev_status;		/* link status of switch ports */
	uint32	pwrsave_mode_manual; 	/* bitmap of ports in manual power save */
	uint32	pwrsave_mode_auto; 	/* bitmap of ports in auto power save mode */
	uint8	pwrsave_phys; 		/* Phys that can be put into power save mode */
	uint8	pwrsave_mode_phys[MAX_NO_PHYS];         /* Power save mode on the switch */   
};

/* Power Save mode related functions */
extern int32 robo_power_save_mode_get(robo_info_t *robo, int32 phy);
extern int32 robo_power_save_mode_set(robo_info_t *robo, int32 mode, int32 phy);
extern void robo_power_save_mode_update(robo_info_t *robo, bool allports);
extern int robo_power_save_mode(robo_info_t *robo, int mode, int phy);
extern int robo_power_save_toggle(robo_info_t *robo, int normal);

extern robo_info_t *bcm_robo_attach(si_t *sih, void *h, char *vars, miird_f miird, miiwr_f miiwr);
extern void bcm_robo_detach(robo_info_t *robo);
extern int bcm_robo_enable_device(robo_info_t *robo);
extern int bcm_robo_config_vlan(robo_info_t *robo, uint8 *mac_addr);
extern int bcm_robo_enable_switch(robo_info_t *robo);


#endif /* _bcm_robo_h_ */
