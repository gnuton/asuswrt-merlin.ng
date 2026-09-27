/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __QCOM_Q6V5_H__
#define __QCOM_Q6V5_H__

#include <linux/kernel.h>
#include <linux/completion.h>
#include <linux/soc/qcom/smem.h>

struct rproc;
struct qcom_smem_state;

struct qcom_q6v5 {
	struct device *dev;
	struct rproc *rproc;

	struct qcom_smem_state *state;
	unsigned stop_bit;
	struct qcom_smem_state *shutdown_state;
	unsigned shutdown_bit;
	struct qcom_smem_state *spawn_state;
	unsigned spawn_bit;

	int wdog_irq;
	int fatal_irq;
	int ready_irq;
	int handover_irq;
	int stop_irq;
	int spawn_irq;

	bool handover_issued;

	struct completion start_done;
	struct completion stop_done;
	struct completion spawn_done;

	bool start_ack;
	bool stop_ack;
	bool spawn_ack;

	int remote_id;
	int crash_reason;

	bool running;

	void (*handover)(struct qcom_q6v5 *q6v5);
};

int qcom_q6v5_init(struct qcom_q6v5 *q6v5, struct platform_device *pdev,
		   struct rproc *rproc, int remote_id, int crash_reason,
		   void (*handover)(struct qcom_q6v5 *q6v5));

int qcom_q6v5_prepare(struct qcom_q6v5 *q6v5);
int qcom_q6v5_unprepare(struct qcom_q6v5 *q6v5);
int qcom_q6v5_request_stop(struct qcom_q6v5 *q6v5);
int qcom_q6v5_request_spawn(struct qcom_q6v5 *q6v5);
int qcom_q6v5_wait_for_start(struct qcom_q6v5 *q6v5, int timeout);
void qcom_q6v5_panic_handler(struct qcom_q6v5 *q6v5);

irqreturn_t q6v5_fatal_interrupt(int irq, void *data);
irqreturn_t q6v5_ready_interrupt(int irq, void *data);
irqreturn_t q6v5_stop_interrupt(int irq, void *data);
irqreturn_t q6v5_spawn_interrupt(int irq, void *data);

#endif
