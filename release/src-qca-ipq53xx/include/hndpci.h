/*
 * HND SiliconBackplane PCI core software interface.
 *
 * $Id: hndpci.h,v 13.8.2.1 2008/10/13 15:51:38 Exp $
 * Copyright (C) 2009, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 */

#ifndef _hndpci_h_
#define _hndpci_h_

extern int hndpci_read_config(si_t *sih, uint bus, uint dev, uint func, uint off, void *buf,
                              int len);
extern int extpci_read_config(si_t *sih, uint bus, uint dev, uint func, uint off, void *buf,
                              int len);
extern int hndpci_write_config(si_t *sih, uint bus, uint dev, uint func, uint off, void *buf,
                               int len);
extern int extpci_write_config(si_t *sih, uint bus, uint dev, uint func, uint off, void *buf,
                               int len);
extern void hndpci_ban(uint16 core);
extern int hndpci_init(si_t *sih);
extern int hndpci_init_pci(si_t *sih);
extern void hndpci_init_cores(si_t *sih);
extern void hndpci_arb_park(si_t *sih, uint parkid);
extern uint8 hndpci_find_pci_capability(si_t *sih, uint bus, uint dev, uint func,
                                        uint8 req_cap_id, uchar *buf, uint32 *buflen);

#define PCI_PARK_NVRAM    0xff

#endif /* _hndpci_h_ */
