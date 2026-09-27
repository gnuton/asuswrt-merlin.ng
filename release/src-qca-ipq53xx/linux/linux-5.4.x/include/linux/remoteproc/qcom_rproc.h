#ifndef __QCOM_RPROC_H__
#define __QCOM_RPROC_H__

struct notifier_block;

#if IS_ENABLED(CONFIG_QCOM_RPROC_COMMON)

int qcom_register_ssr_notifier(struct notifier_block *nb);
void qcom_unregister_ssr_notifier(struct notifier_block *nb);
char **qcom_get_ssr_subdev_name(struct device *dev, int *num_subdev);
#else

static inline int qcom_register_ssr_notifier(struct notifier_block *nb)
{
	return 0;
}

static inline void qcom_unregister_ssr_notifier(struct notifier_block *nb) {}

static inline char **qcom_get_ssr_subdev_name(struct device *dev,
							int *num_subdev)
{
	return NULL;
}
#endif

#endif
