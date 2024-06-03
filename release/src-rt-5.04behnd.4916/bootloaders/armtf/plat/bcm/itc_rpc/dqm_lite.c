/* SPDX-License-Identifier: GPL-2.0+ */
/***************************************************************************
 * Broadcom Proprietary and Confidential. (c)2019 Broadcom. All rights reserved.
 *
 *  THIS SOFTWARE MAY ONLY BE USED SUBJECT TO AN EXECUTED SOFTWARE LICENSE
 *  AGREEMENT  BETWEEN THE USER AND BROADCOM.  YOU HAVE NO RIGHT TO USE OR
 *  EXPLOIT THIS MATERIAL EXCEPT SUBJECT TO THE TERMS OF SUCH AN AGREEMENT.
 *
 ***************************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <platform_def.h>
#include "dqm_lite.h"

struct dqm {
	uint64_t status;
	uint64_t data;
	uint16_t tkn_size;
};

struct dqm_device {
	int (*open)(int q_idx);
	uint32_t (*avl_tkn_space)(struct dqm *q);
	int num_q;
	struct dqm *q;
};

#define MAKE_Q_ID(d, q)	((((d) & 0xff) << 8) | ((q) & 0xff))
#define DEV_IDX(id)	(((id) >> 8) & 0xff)
#define Q_IDX(id)	((id) & 0xff)

/* SMC specific */
static uint32_t q_status_off;
static uint32_t q_data_off;
static volatile uint8_t *reg_base;
static uint32_t smc_avl_tkn_space(struct dqm *q);
static int smc_dqm_open(int q_idx);

#define SMC_DQM_IRQ_NOT_EMPTY_STATUS			0x1D00

#define SMC_QUEUE_STATUS_STSi_Q_AVL_TKN_SPACE_MASK 	0x00003fff
#define SMC_QUEUE_STATUS_STSi_Q_AVL_TKN_SPACE_SHIFT	0

#define SMC_Q_DATA(qid) (q_data_off + ((qid) * 16))

#define SMC_Q_STATUS(qid) (q_status_off + ((qid) * 4))
	
#define SMC_Q_NE_STATUS(qid) (*(uint32_t *)(reg_base + SMC_DQM_IRQ_NOT_EMPTY_STATUS) & (1 << qid))


/* These must match enum dqm_dev_idx DQM_DEV_xxx #defines in dqm_lite.h */
static struct dqm_device device[NUM_DQM_DEVS] =
{
	{ /* SMC */
		.open = smc_dqm_open,
		.avl_tkn_space = smc_avl_tkn_space,
		.num_q = 0,
		.q = NULL
	},
};

static uint32_t smc_avl_tkn_space(struct dqm *q)
{
	uint32_t qsts = *(uint32_t *)(reg_base + q->status);
	qsts = (qsts & SMC_QUEUE_STATUS_STSi_Q_AVL_TKN_SPACE_MASK) >> SMC_QUEUE_STATUS_STSi_Q_AVL_TKN_SPACE_SHIFT;
	return qsts;
}

static int smc_dqm_open(int q_idx)
{
	struct dqm_device *dev = &device[DQM_DEV_SMC];
	struct dqm *q = dev->q + q_idx;

	q->status = SMC_Q_STATUS(q_idx);
	q->data = SMC_Q_DATA(q_idx);
	q->tkn_size = SMC_WORDS_PER_TOKEN;
	return MAKE_Q_ID(DQM_DEV_SMC, q_idx);
}




/* API for any DQM device */
int dqm_open(enum dqm_dev_idx dev_idx, int q_idx)
{
	if (dev_idx < 0 || dev_idx >= NUM_DQM_DEVS ||
	    q_idx < 0 || q_idx >= device[dev_idx].num_q)
		return -1;
	return device[dev_idx].open(q_idx);
}

int dqm_read(int q_id, unsigned char *buffer, int count)
{
	uint32_t *words = (uint32_t *)buffer;
	int i, j, words_per_msg;
	struct dqm_device *dev;
	struct dqm *q;
	int dev_idx, q_idx;

	dev_idx = DEV_IDX(q_id);
	q_idx = Q_IDX(q_id);
	if (dev_idx < 0 || dev_idx >= NUM_DQM_DEVS ||
	    q_idx < 0 || q_idx >= device[dev_idx].num_q)
		return 0;
	dev = &device[dev_idx];
	q = dev->q + q_idx;
	words_per_msg = q->tkn_size;

	i=0;
	while(i < count) {
		if (SMC_Q_NE_STATUS(q_idx))	{
			for (j = 0; j < words_per_msg; j++)
				words[i * words_per_msg + j] = 	*(uint32_t *)(reg_base + q->data + (j * 4));
			i++;
		} else
			break;
	}
	return i;
}

int dqm_write(int q_id, unsigned char *buffer, int count)
{
	uint32_t i, j, qsts, words_per_msg, *words = (uint32_t *)buffer;
	struct dqm_device *dev;
	struct dqm *q;
	int dev_idx, q_idx;
	uint32_t cnt = (uint32_t)count;

	dev_idx = DEV_IDX(q_id);
	q_idx = Q_IDX(q_id);
	if (dev_idx < 0 || dev_idx >= NUM_DQM_DEVS ||
	    q_idx < 0 || q_idx >= device[dev_idx].num_q)
		return 0;
	dev = &device[dev_idx];
	q = dev->q + q_idx;
	qsts = dev->avl_tkn_space(q);
	qsts = qsts < cnt ? qsts : cnt;
	words_per_msg = q->tkn_size;
	for (i = 0; i < qsts; i++) {
		for (j = 0; j < words_per_msg; j++) {
			*(uint32_t *)(reg_base + q->data + (j * 4)) = words[i * words_per_msg + j];
		}
	}

	return qsts;
}

void bcm_dqm_init(void)
{
	static struct dqm smc_q[DQM_Q_COUNT];

	reg_base = (uint8_t *)SMC_BASE;
	q_status_off = DQM_STATUS_OFFSET;
	q_data_off = DQM_DATA_OFFSET;
	device[DQM_DEV_SMC].num_q = DQM_Q_COUNT;
	device[DQM_DEV_SMC].q = smc_q;
}

