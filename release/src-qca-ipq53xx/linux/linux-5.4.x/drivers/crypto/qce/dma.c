// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2012-2014, 2021 The Linux Foundation. All rights reserved.
 */

#include <linux/dmaengine.h>
#include <crypto/scatterwalk.h>

#include "dma.h"
#include "core.h"

#define QCE_REG_BUF_DMA_ADDR(qce, vaddr) \
	((qce)->reg_read_buf_phys + \
	 ((uint8_t *)(vaddr) - (uint8_t *)(qce)->reg_read_buf))

void qce_clear_bam_transaction(struct qce_device *qce)
{
	struct qce_bam_transaction *qce_bam_txn = qce->dma.qce_bam_txn;
	qce_bam_txn->qce_bam_ce_index = 0;
	qce_bam_txn->qce_write_sgl_cnt = 0;
	qce_bam_txn->qce_read_sgl_cnt = 0;
	qce_bam_txn->qce_bam_ce_index = 0;
	qce_bam_txn->qce_pre_bam_ce_index = 0;
}

static int qce_dma_prep_cmd_sg(struct qce_device *qce, struct dma_chan *chan,
				struct scatterlist *qce_bam_sgl,
				int qce_sgl_cnt, unsigned long flags,
				enum dma_transfer_direction dir,
				dma_async_tx_callback cb, void *cb_param)
{
	struct dma_async_tx_descriptor *dma_desc;
	dma_cookie_t cookie;
	struct qce_desc_info *desc;

	desc = qce->dma.qce_bam_txn->qce_desc;

	if (!qce_bam_sgl || !qce_sgl_cnt)
		return -EINVAL;

	if (!dma_map_sg(qce->dev, qce_bam_sgl,
				qce_sgl_cnt, dir)) {
		dev_err(qce->dev, "failure in mapping sgl for cmd desc\n");
		return -ENOMEM;
	}

	dma_desc = dmaengine_prep_slave_sg(chan, qce_bam_sgl, qce_sgl_cnt,
						dir, flags);
	if (!dma_desc) {
		pr_err("%s:failure in prep cmd desc\n",__func__);
		dma_unmap_sg(qce->dev, qce_bam_sgl, qce_sgl_cnt, dir);
		kfree(desc);
		return -EINVAL;
	}

	desc->dma_desc = dma_desc;
	desc->dma_desc->callback = cb;
	desc->dma_desc->callback_param = cb_param;

	cookie = dmaengine_submit(desc->dma_desc);

	return dma_submit_error(cookie);
}

int qce_submit_cmd_desc(struct qce_device *qce, unsigned long flags)
{
	struct qce_bam_transaction *qce_bam_txn = qce->dma.qce_bam_txn;
	struct dma_chan *chan = qce->dma.rxchan;
	unsigned long desc_flags;
	int ret = 0;

	if(flags & QCE_DMA_DESC_FLAG_LOCK)
		desc_flags = DMA_PREP_CMD | DMA_PREP_LOCK;
	else if (flags & QCE_DMA_DESC_FLAG_UNLOCK)
		desc_flags = DMA_PREP_CMD | DMA_PREP_UNLOCK;
	else
		desc_flags = DMA_PREP_CMD;

	/* For command descriptor always use consumer pipe
	 * it recomended as per HPG
	 */

	if (qce_bam_txn->qce_read_sgl_cnt) {
		ret = qce_dma_prep_cmd_sg(qce, chan,
					qce_bam_txn->qce_reg_read_sgl,
					qce_bam_txn->qce_read_sgl_cnt,
					desc_flags, DMA_DEV_TO_MEM,
					NULL, NULL);
	}

	if (qce_bam_txn->qce_write_sgl_cnt) {
		ret = qce_dma_prep_cmd_sg(qce, chan,
					qce_bam_txn->qce_reg_write_sgl,
					qce_bam_txn->qce_write_sgl_cnt,
					desc_flags, DMA_MEM_TO_DEV,
					NULL, NULL);
	}

	if (ret) {
		pr_err("%s: Error while submiting cmd desc\n",__func__);
		return ret;
	}

	qce_dma_issue_pending(&qce->dma);

	return ret;
}

static void qce_prep_dma_command_desc(struct qce_device *qce,
		struct qce_dma_data *dma, bool read, unsigned int addr,
		void *buff, int size)
{
	int qce_bam_ce_size, cnt, index;
	struct bam_cmd_element *qce_bam_ce_buffer;
	struct qce_bam_transaction *qce_bam_txn = dma->qce_bam_txn;

	index = qce_bam_txn->qce_bam_ce_index;
	qce_bam_ce_buffer = &qce_bam_txn->qce_bam_ce[index];
	if (read)
		bam_prep_ce(qce_bam_ce_buffer, addr, BAM_READ_COMMAND,
				QCE_REG_BUF_DMA_ADDR(qce,
					(unsigned int *)buff));
	else
		bam_prep_ce_le32(qce_bam_ce_buffer, addr, BAM_WRITE_COMMAND,
				 *((__le32 *)buff));

	if (read) {
		cnt = qce_bam_txn->qce_read_sgl_cnt;
		qce_bam_ce_buffer = &qce_bam_txn->qce_bam_ce
			[qce_bam_txn->qce_pre_bam_ce_index];
		qce_bam_txn->qce_bam_ce_index += size;
		qce_bam_ce_size = (qce_bam_txn->qce_bam_ce_index -
				qce_bam_txn->qce_pre_bam_ce_index) *
			sizeof(struct bam_cmd_element);

		sg_set_buf(&qce_bam_txn->qce_reg_read_sgl[cnt],
				qce_bam_ce_buffer,
				qce_bam_ce_size);

		++qce_bam_txn->qce_read_sgl_cnt;
		qce_bam_txn->qce_pre_bam_ce_index =
					qce_bam_txn->qce_bam_ce_index;
	} else {
		cnt = qce_bam_txn->qce_write_sgl_cnt;
		qce_bam_ce_buffer = &qce_bam_txn->qce_bam_ce
			[qce_bam_txn->qce_pre_bam_ce_index];
		qce_bam_txn->qce_bam_ce_index += size;
		qce_bam_ce_size = (qce_bam_txn->qce_bam_ce_index -
				qce_bam_txn->qce_pre_bam_ce_index) *
			sizeof(struct bam_cmd_element);

		sg_set_buf(&qce_bam_txn->qce_reg_write_sgl[cnt],
				qce_bam_ce_buffer,
				qce_bam_ce_size);

		++qce_bam_txn->qce_write_sgl_cnt;
		qce_bam_txn->qce_pre_bam_ce_index =
					qce_bam_txn->qce_bam_ce_index;
	}
}

int qce_write_reg_dma(struct qce_device *qce,
		unsigned int offset, u32 val, int cnt)
{
	void *buff = &val;
	unsigned int reg_addr = ((unsigned int)(qce->base_dma) + offset);

	qce_prep_dma_command_desc(qce, &qce->dma, false, reg_addr, buff, cnt);

	return 0;
}

int qce_read_reg_dma(struct qce_device *qce,
		unsigned int offset, void *buff, int cnt)
{
	unsigned int reg_addr = ((unsigned int)(qce->base_dma) + offset);
	void *vaddr = qce->reg_read_buf;

	qce_prep_dma_command_desc(qce, &qce->dma, true, reg_addr, vaddr, cnt);

	memcpy(buff, vaddr, 4);

	return 0;
}

struct qce_bam_transaction *qce_alloc_bam_txn(struct qce_dma_data *dma)
{
	struct qce_bam_transaction *qce_bam_txn;

	dma->qce_bam_txn = kmalloc(sizeof(*qce_bam_txn), GFP_KERNEL);
	if (!dma->qce_bam_txn)
		return NULL;

	dma->qce_bam_txn->qce_desc = kzalloc(sizeof(struct qce_desc_info),
						GFP_KERNEL);
	if (!dma->qce_bam_txn->qce_desc)
		return NULL;

	sg_init_table(dma->qce_bam_txn->qce_reg_write_sgl,
			QCE_BAM_CMD_SGL_SIZE);

	sg_init_table(dma->qce_bam_txn->qce_reg_read_sgl,
			QCE_BAM_CMD_SGL_SIZE);

	qce_bam_txn = dma->qce_bam_txn;

	return qce_bam_txn;
}

int qce_dma_request(struct device *dev, struct qce_dma_data *dma)
{
	int ret;
	struct qce_device *qce = container_of(dma, struct qce_device, dma);

	dma->txchan = dma_request_chan(dev, "tx");
	if (IS_ERR(dma->txchan))
		return PTR_ERR(dma->txchan);

	dma->rxchan = dma_request_chan(dev, "rx");
	if (IS_ERR(dma->rxchan)) {
		ret = PTR_ERR(dma->rxchan);
		goto error_rx;
	}

	dma->result_buf = kmalloc(QCE_RESULT_BUF_SZ + QCE_IGNORE_BUF_SZ,
				  GFP_KERNEL);
	if (!dma->result_buf) {
		ret = -ENOMEM;
		goto error_nomem;
	}

	dma->ignore_buf = dma->result_buf + QCE_RESULT_BUF_SZ;

	if (qce->qce_cmd_desc_enable) {
		dma->qce_bam_txn = qce_alloc_bam_txn(dma);
		if (!dma->qce_bam_txn) {
			pr_err("%s Failed to allocate bam transaction\n",
				__func__);
			return -ENOMEM;
		}

		qce->reg_read_buf = dmam_alloc_coherent(qce->dev,
					QCE_MAX_REG_READ *
					sizeof(*qce->reg_read_buf),
					&qce->reg_read_buf_phys, GFP_KERNEL);
		if (!qce->reg_read_buf) {
			pr_err("%s: Failed to allocate reg_read_buf\n",
				__func__);
			return -ENOMEM;
		}
	}

	return 0;
error_nomem:
	dma_release_channel(dma->rxchan);
error_rx:
	dma_release_channel(dma->txchan);
	return ret;
}

void qce_dma_release(struct qce_dma_data *dma)
{
	struct qce_device *qce = container_of(dma,
			struct qce_device, dma);

	dma_release_channel(dma->txchan);
	dma_release_channel(dma->rxchan);
	kfree(dma->result_buf);
	if (qce->qce_cmd_desc_enable) {
		if (qce->reg_read_buf)
			dmam_free_coherent(qce->dev, QCE_MAX_REG_READ *
				sizeof(*qce->reg_read_buf),
				qce->reg_read_buf,
				qce->reg_read_buf_phys);
		kfree(dma->qce_bam_txn->qce_desc);
		kfree(dma->qce_bam_txn);
	}
}

struct scatterlist *
qce_sgtable_add(struct sg_table *sgt, struct scatterlist *new_sgl,
		unsigned int max_len)
{
	struct scatterlist *sg = sgt->sgl, *sg_last = NULL;
	unsigned int new_len;

	while (sg) {
		if (!sg_page(sg))
			break;
		sg = sg_next(sg);
	}

	if (!sg)
		return ERR_PTR(-EINVAL);

	while (new_sgl && sg && max_len) {
		new_len = new_sgl->length > max_len ? max_len : new_sgl->length;
		sg_set_page(sg, sg_page(new_sgl), new_len, new_sgl->offset);
		sg_last = sg;
		sg = sg_next(sg);
		new_sgl = sg_next(new_sgl);
		max_len -= new_len;
	}

	if (!sg_last)
		return ERR_PTR(-EINVAL);

	return sg_last;
}

static int qce_dma_prep_sg(struct dma_chan *chan, struct scatterlist *sg,
		int nents, unsigned long flags,
		enum dma_transfer_direction dir,
		dma_async_tx_callback cb, void *cb_param)
{
	struct dma_async_tx_descriptor *desc;
	dma_cookie_t cookie;
	if (!sg || !nents)
		return -EINVAL;
	desc = dmaengine_prep_slave_sg(chan, sg, nents, dir, flags);
	if (!desc)
		return -EINVAL;

	desc->callback = cb;
	desc->callback_param = cb_param;
	cookie = dmaengine_submit(desc);

	return dma_submit_error(cookie);
}

int qce_dma_prep_sgs(struct qce_dma_data *dma, struct scatterlist *rx_sg,
		     int rx_nents, struct scatterlist *tx_sg, int tx_nents,
		     dma_async_tx_callback cb, void *cb_param)
{
	struct dma_chan *rxchan = dma->rxchan;
	struct dma_chan *txchan = dma->txchan;
	unsigned long flags = DMA_PREP_INTERRUPT | DMA_CTRL_ACK;
	int ret;

	ret = qce_dma_prep_sg(rxchan, rx_sg, rx_nents, flags, DMA_MEM_TO_DEV,
			NULL, NULL);
	if (ret)
		return ret;

	return qce_dma_prep_sg(txchan, tx_sg, tx_nents, flags, DMA_DEV_TO_MEM,
			cb, cb_param);
}

void qce_dma_issue_pending(struct qce_dma_data *dma)
{
	dma_async_issue_pending(dma->rxchan);
	dma_async_issue_pending(dma->txchan);
}

int qce_dma_terminate_all(struct qce_dma_data *dma)
{
	int ret;

	ret = dmaengine_terminate_all(dma->rxchan);
	return ret ?: dmaengine_terminate_all(dma->txchan);
}
