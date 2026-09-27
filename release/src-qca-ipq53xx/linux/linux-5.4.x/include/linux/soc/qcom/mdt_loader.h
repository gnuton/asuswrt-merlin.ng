/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __QCOM_MDT_LOADER_H__
#define __QCOM_MDT_LOADER_H__

#include <linux/types.h>

#define QCOM_MDT_TYPE_MASK	(7 << 24)
#define QCOM_MDT_TYPE_HASH	(2 << 24)
#define QCOM_MDT_RELOCATABLE	BIT(27)
#define QCOM_MDT_ASID_MASK      0xfu
#define QCOM_MDT_PF_ASID_SHIFT  16
#define QCOM_MDT_PF_ASID_MASK   (QCOM_MDT_ASID_MASK << QCOM_MDT_PF_ASID_SHIFT)
#define QCOM_MDT_PF_ASID(x)     \
			(((x) >> QCOM_MDT_PF_ASID_SHIFT) & QCOM_MDT_ASID_MASK)

struct device;
struct firmware;

struct qcom_pd_fw_info {
	phys_addr_t paddr;
	void *vaddr;
	size_t size;
};

ssize_t qcom_mdt_get_file_size(const struct firmware *fw);
ssize_t qcom_mdt_get_size(const struct firmware *fw);
int qcom_mdt_load(struct device *dev, const struct firmware *fw,
		  const char *fw_name, int pas_id, void *mem_region,
		  phys_addr_t mem_phys, size_t mem_size,
		  phys_addr_t *reloc_base);

int qcom_mdt_load_no_init(struct device *dev, const struct firmware *fw,
			  const char *fw_name, int pas_id, void *mem_region,
			  phys_addr_t mem_phys, size_t mem_size,
			  phys_addr_t *reloc_base);
int qcom_mdt_load_pd_seg(struct device *dev, const struct firmware *fw,
		  const char *fw_name, int pas_id, void *mem_region,
		  phys_addr_t mem_phys, size_t mem_size,
		  phys_addr_t *reloc_base);

int qcom_mdt_load_pd_seg_no_init(struct device *dev, const struct firmware *fw,
			  const char *fw_name, int pas_id, void *mem_region,
			  phys_addr_t mem_phys, size_t mem_size,
			  phys_addr_t *reloc_base);
void *qcom_mdt_read_metadata(const struct firmware *fw, size_t *data_len);
int get_pd_fw_info(struct device *dev, const struct firmware *fw,
			phys_addr_t mem_phys, size_t mem_size, u8 pd_asid,
			struct qcom_pd_fw_info *fw_info);
#endif
