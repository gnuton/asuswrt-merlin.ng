// SPDX-License-Identifier: GPL-2.0
/*
 * Qualcomm Peripheral Image Loader for Q6V5
 *
 * Copyright (C) 2016-2018 Linaro Ltd.
 * Copyright (C) 2014 Sony Mobile Communications AB
 * Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 */
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/soc/qcom/smem.h>
#include <linux/soc/qcom/smem_state.h>
#include <linux/remoteproc.h>
#include "qcom_q6v5.h"
#include <linux/delay.h>

#define STOP_ACK_TIMEOUT_MS 5000

/**
 * qcom_q6v5_prepare() - reinitialize the qcom_q6v5 context before start
 * @q6v5:	reference to qcom_q6v5 context to be reinitialized
 *
 * Return: 0 on success, negative errno on failure
 */
int qcom_q6v5_prepare(struct qcom_q6v5 *q6v5)
{
	reinit_completion(&q6v5->start_done);
	reinit_completion(&q6v5->stop_done);
	reinit_completion(&q6v5->spawn_done);

	q6v5->running = true;
	q6v5->handover_issued = false;
	q6v5->start_ack = false;
	q6v5->stop_ack = false;
	q6v5->spawn_ack = false;

	enable_irq(q6v5->handover_irq);

	return 0;
}
EXPORT_SYMBOL_GPL(qcom_q6v5_prepare);

/**
 * qcom_q6v5_unprepare() - unprepare the qcom_q6v5 context after stop
 * @q6v5:	reference to qcom_q6v5 context to be unprepared
 *
 * Return: 0 on success, 1 if handover hasn't yet been called
 */
int qcom_q6v5_unprepare(struct qcom_q6v5 *q6v5)
{
	disable_irq(q6v5->handover_irq);

	return !q6v5->handover_issued;
}
EXPORT_SYMBOL_GPL(qcom_q6v5_unprepare);

static irqreturn_t q6v5_wdog_interrupt(int irq, void *data)
{
	struct qcom_q6v5 *q6v5 = data;
	size_t len;
	char *msg;

	/* Sometimes the stop triggers a watchdog rather than a stop-ack */
	if (!q6v5->running) {
		q6v5->stop_ack = true;
		complete(&q6v5->stop_done);
		return IRQ_HANDLED;
	}

	msg = qcom_smem_get(q6v5->remote_id, q6v5->crash_reason, &len);
	if (!IS_ERR(msg) && len > 0 && msg[0])
		dev_err(q6v5->dev, "watchdog received: %s\n", msg);
	else
		dev_err(q6v5->dev, "watchdog without message\n");

	/* Complete any pending waits for this rproc */
	complete(&q6v5->spawn_done);
	complete(&q6v5->start_done);
	complete(&q6v5->stop_done);
	rproc_report_crash(q6v5->rproc, RPROC_WATCHDOG);

	return IRQ_HANDLED;
}

irqreturn_t q6v5_fatal_interrupt(int irq, void *data)
{
	struct qcom_q6v5 *q6v5 = data;
	size_t len;
	char *msg;

	msg = qcom_smem_get(q6v5->remote_id, q6v5->crash_reason, &len);
	if (!IS_ERR(msg) && len > 0 && msg[0])
		dev_err(q6v5->dev, "fatal error received: %s\n", msg);
	else
		dev_err(q6v5->dev, "fatal error without message\n");

	q6v5->running = false;

	/* Complete any pending waits for this rproc */
	complete(&q6v5->spawn_done);
	complete(&q6v5->start_done);
	complete(&q6v5->stop_done);

	rproc_report_crash(q6v5->rproc, RPROC_FATAL_ERROR);

	return IRQ_HANDLED;
}

irqreturn_t q6v5_ready_interrupt(int irq, void *data)
{
	struct qcom_q6v5 *q6v5 = data;

	pr_info("Subsystem error monitoring/handling services are up\n");

	q6v5->start_ack = true;
	complete(&q6v5->start_done);

	return IRQ_HANDLED;
}

/**
 * qcom_q6v5_wait_for_start() - wait for remote processor start signal
 * @q6v5:	reference to qcom_q6v5 context
 * @timeout:	timeout to wait for the event, in jiffies
 *
 * qcom_q6v5_unprepare() should not be called when this function fails.
 *
 * Return: 0 on success, -ETIMEDOUT on timeout
 */
int qcom_q6v5_wait_for_start(struct qcom_q6v5 *q6v5, int timeout)
{
	int ret;

	ret = wait_for_completion_timeout(&q6v5->start_done, timeout);

	if (!ret) {
		disable_irq(q6v5->handover_irq);
		return -ETIMEDOUT;
	} else {
		return q6v5->start_ack ? 0 : -ERESTARTSYS;
	}
}
EXPORT_SYMBOL_GPL(qcom_q6v5_wait_for_start);

static irqreturn_t q6v5_handover_interrupt(int irq, void *data)
{
	struct qcom_q6v5 *q6v5 = data;

	if (q6v5->handover)
		q6v5->handover(q6v5);

	q6v5->handover_issued = true;

	return IRQ_HANDLED;
}

irqreturn_t q6v5_spawn_interrupt(int irq, void *data)
{
	struct qcom_q6v5 *q6v5 = data;

	q6v5->spawn_ack = true;
	complete(&q6v5->spawn_done);

	return IRQ_HANDLED;
}

irqreturn_t q6v5_stop_interrupt(int irq, void *data)
{
	struct qcom_q6v5 *q6v5 = data;

	q6v5->stop_ack = true;
	complete(&q6v5->stop_done);

	return IRQ_HANDLED;
}

/**
 * qcom_q6v5_request_stop() - request the remote processor to stop
 * @q6v5:	reference to qcom_q6v5 context
 *
 * Return: 0 on success, negative errno on failure
 */
int qcom_q6v5_request_stop(struct qcom_q6v5 *q6v5)
{
	int ret;

	q6v5->running = false;
	q6v5->stop_ack = false;

	qcom_smem_state_update_bits(q6v5->state,
			BIT(q6v5->stop_bit), BIT(q6v5->stop_bit));

	ret = wait_for_completion_timeout(&q6v5->stop_done,
						msecs_to_jiffies(10000));

	qcom_smem_state_update_bits(q6v5->state, BIT(q6v5->stop_bit), 0);

	if (!ret)
		return -ETIMEDOUT;
	else
		return q6v5->stop_ack ? 0 : -ERESTARTSYS;
}
EXPORT_SYMBOL_GPL(qcom_q6v5_request_stop);

/**
 * qcom_q6v5_request_spawn() - request the remote processor to spawn
 * @q6v5:      reference to qcom_q6v5 context
 *
 * Return: 0 on success, negative errno on failure
 */
int qcom_q6v5_request_spawn(struct qcom_q6v5 *q6v5)
{
	int ret;

	q6v5->spawn_ack = false;
	ret = qcom_smem_state_update_bits(q6v5->spawn_state,
			BIT(q6v5->spawn_bit), BIT(q6v5->spawn_bit));

	ret = wait_for_completion_timeout(&q6v5->spawn_done,
						msecs_to_jiffies(10000));

	qcom_smem_state_update_bits(q6v5->spawn_state,
						BIT(q6v5->spawn_bit), 0);

	if (!ret)
		return -ETIMEDOUT;
	else
		return q6v5->spawn_ack ? 0 : -ERESTARTSYS;
}
EXPORT_SYMBOL_GPL(qcom_q6v5_request_spawn);

/**
 * qcom_q6v5_panic_handler() - stop remote processor on panic
 * @q6v5:	reference to qcom_q6v5 context
 *
 */
void qcom_q6v5_panic_handler(struct qcom_q6v5 *q6v5)
{
	q6v5->running = false;

	smem_panic_handler();
	qcom_smem_state_update_bits(q6v5->shutdown_state,
			BIT(q6v5->shutdown_bit), BIT(q6v5->shutdown_bit));
	qcom_log_smp2p_ob_cmd(q6v5->shutdown_bit, BIT(q6v5->shutdown_bit),
			      BIT(q6v5->shutdown_bit));
	pr_info("APSS Panic: Sent shutdown request to Q6\n");
	mdelay(STOP_ACK_TIMEOUT_MS);
}
EXPORT_SYMBOL_GPL(qcom_q6v5_panic_handler);

/**
 * qcom_q6v5_init() - initializer of the q6v5 common struct
 * @q6v5:	handle to be initialized
 * @pdev:	platform_device reference for acquiring resources
 * @rproc:	associated remoteproc instance
 * @crash_reason: SMEM id for crash reason string, or 0 if none
 * @handover:	function to be called when proxy resources should be released
 *
 * Return: 0 on success, negative errno on failure
 */
int qcom_q6v5_init(struct qcom_q6v5 *q6v5, struct platform_device *pdev,
		   struct rproc *rproc, int remote_id, int crash_reason,
		   void (*handover)(struct qcom_q6v5 *q6v5))
{
	int ret;

	q6v5->rproc = rproc;
	q6v5->dev = &pdev->dev;
	q6v5->crash_reason = crash_reason;
	q6v5->handover = handover;
	q6v5->remote_id = remote_id;

	init_completion(&q6v5->start_done);
	init_completion(&q6v5->stop_done);
	init_completion(&q6v5->spawn_done);

	q6v5->wdog_irq = platform_get_irq_byname(pdev, "wdog");
	if (q6v5->wdog_irq < 0)
		return q6v5->wdog_irq;

	ret = devm_request_threaded_irq(&pdev->dev, q6v5->wdog_irq,
					NULL, q6v5_wdog_interrupt,
					IRQF_TRIGGER_RISING | IRQF_ONESHOT,
					"q6v5 wdog", q6v5);
	if (ret) {
		dev_err(&pdev->dev, "failed to acquire wdog IRQ\n");
		return ret;
	}

	q6v5->fatal_irq = platform_get_irq_byname(pdev, "fatal");
	if (q6v5->fatal_irq < 0)
		return q6v5->fatal_irq;

	ret = devm_request_threaded_irq(&pdev->dev, q6v5->fatal_irq,
					NULL, q6v5_fatal_interrupt,
					IRQF_TRIGGER_RISING | IRQF_ONESHOT,
					"q6v5 fatal", q6v5);
	if (ret) {
		dev_err(&pdev->dev, "failed to acquire fatal IRQ\n");
		return ret;
	}

	q6v5->ready_irq = platform_get_irq_byname(pdev, "ready");
	if (q6v5->ready_irq < 0)
		return q6v5->ready_irq;

	ret = devm_request_threaded_irq(&pdev->dev, q6v5->ready_irq,
					NULL, q6v5_ready_interrupt,
					IRQF_TRIGGER_RISING | IRQF_ONESHOT,
					"q6v5 ready", q6v5);
	if (ret) {
		dev_err(&pdev->dev, "failed to acquire ready IRQ\n");
		return ret;
	}

	q6v5->handover_irq = platform_get_irq_byname(pdev, "handover");
	if (q6v5->handover_irq < 0)
		return q6v5->handover_irq;

	ret = devm_request_threaded_irq(&pdev->dev, q6v5->handover_irq,
					NULL, q6v5_handover_interrupt,
					IRQF_TRIGGER_RISING | IRQF_ONESHOT,
					"q6v5 handover", q6v5);
	if (ret) {
		dev_err(&pdev->dev, "failed to acquire handover IRQ\n");
		return ret;
	}
	disable_irq(q6v5->handover_irq);

	q6v5->stop_irq = platform_get_irq_byname(pdev, "stop-ack");
	if (q6v5->stop_irq < 0)
		return q6v5->stop_irq;

	ret = devm_request_threaded_irq(&pdev->dev, q6v5->stop_irq,
					NULL, q6v5_stop_interrupt,
					IRQF_TRIGGER_RISING | IRQF_ONESHOT,
					"q6v5 stop", q6v5);
	if (ret) {
		dev_err(&pdev->dev, "failed to acquire stop-ack IRQ\n");
		return ret;
	}

	q6v5->state = qcom_smem_state_get(&pdev->dev, "stop", &q6v5->stop_bit);
	if (IS_ERR(q6v5->state)) {
		dev_err(&pdev->dev, "failed to acquire stop state\n");
		return PTR_ERR(q6v5->state);
	}

	q6v5->shutdown_state = qcom_smem_state_get(&pdev->dev, "shutdown", &q6v5->shutdown_bit);
	if (IS_ERR(q6v5->shutdown_state)) {
		dev_err(&pdev->dev, "failed to acquire shutdown state\n");
		return PTR_ERR(q6v5->shutdown_state);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(qcom_q6v5_init);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Qualcomm Peripheral Image Loader for Q6V5");
