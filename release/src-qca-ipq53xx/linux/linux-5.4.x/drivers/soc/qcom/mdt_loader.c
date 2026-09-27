// SPDX-License-Identifier: GPL-2.0-only
/*
 * Qualcomm Peripheral Image Loader
 *
 * Copyright (C) 2016 Linaro Ltd
 * Copyright (C) 2015 Sony Mobile Communications Inc
 * Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 */

#include <linux/device.h>
#include <linux/elf.h>
#include <linux/firmware.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/qcom_scm.h>
#include <linux/sizes.h>
#include <linux/slab.h>
#include <linux/soc/qcom/mdt_loader.h>
#include <linux/dma-mapping.h>

#include "../../remoteproc/qcom_common.h"
#define PDSEG_PAS_ID	0xD

static bool mdt_phdr_valid(const struct elf32_phdr *phdr)
{
	if (phdr->p_type != PT_LOAD)
		return false;

	if ((phdr->p_flags & QCOM_MDT_TYPE_MASK) == QCOM_MDT_TYPE_HASH)
		return false;

	if (!phdr->p_memsz)
		return false;

	return true;
}

/**
 * qcom_mdt_get_file_size() - acquire size of the file region
 * @fw:		firmware object for the mdt file
 *
 * Returns size of the loaded firmware blob, or -EINVAL on failure.
 */
ssize_t qcom_mdt_get_file_size(const struct firmware *fw)
{
	const struct elf32_phdr *phdrs;
	const struct elf32_phdr *phdr;
	const struct elf32_hdr *ehdr;
	phys_addr_t min_addr = PHYS_ADDR_MAX;
	phys_addr_t max_addr = 0;
	int i;

	ehdr = (struct elf32_hdr *)fw->data;
	phdrs = (struct elf32_phdr *)(ehdr + 1);

	for (i = 0; i < ehdr->e_phnum; i++) {
		phdr = &phdrs[i];

		if (!mdt_phdr_valid(phdr))
			continue;

		if (phdr->p_paddr < min_addr)
			min_addr = phdr->p_paddr;

		if (phdr->p_paddr + phdr->p_filesz > max_addr)
			max_addr = phdr->p_paddr + phdr->p_filesz;
	}

	return min_addr < max_addr ? max_addr - min_addr : -EINVAL;
}
EXPORT_SYMBOL_GPL(qcom_mdt_get_file_size);

/**
 * qcom_mdt_get_size() - acquire size of the memory region needed to load mdt
 * @fw:		firmware object for the mdt file
 *
 * Returns size of the loaded firmware blob, or -EINVAL on failure.
 */
ssize_t qcom_mdt_get_size(const struct firmware *fw)
{
	const struct elf32_phdr *phdrs;
	const struct elf32_phdr *phdr;
	const struct elf32_hdr *ehdr;
	phys_addr_t min_addr = PHYS_ADDR_MAX;
	phys_addr_t max_addr = 0;
	int i;

	ehdr = (struct elf32_hdr *)fw->data;
	phdrs = (struct elf32_phdr *)(ehdr + 1);

	for (i = 0; i < ehdr->e_phnum; i++) {
		phdr = &phdrs[i];

		if (!mdt_phdr_valid(phdr))
			continue;

		if (phdr->p_paddr < min_addr)
			min_addr = phdr->p_paddr;

		if (phdr->p_paddr + phdr->p_memsz > max_addr)
			max_addr = ALIGN(phdr->p_paddr + phdr->p_memsz, SZ_4K);
	}

	return min_addr < max_addr ? max_addr - min_addr : -EINVAL;
}
EXPORT_SYMBOL_GPL(qcom_mdt_get_size);

/**
 * qcom_mdt_read_metadata() - read header and metadata from mdt or mbn
 * @fw:		firmware of mdt header or mbn
 * @data_len:	length of the read metadata blob
 *
 * The mechanism that performs the authentication of the loading firmware
 * expects an ELF header directly followed by the segment of hashes, with no
 * padding inbetween. This function allocates a chunk of memory for this pair
 * and copy the two pieces into the buffer.
 *
 * In the case of split firmware the hash is found directly following the ELF
 * header, rather than at p_offset described by the second program header.
 *
 * The caller is responsible to free (kfree()) the returned pointer.
 *
 * Return: pointer to data, or ERR_PTR()
 */
void *qcom_mdt_read_metadata(const struct firmware *fw, size_t *data_len)
{
	const struct elf32_phdr *phdrs;
	const struct elf32_hdr *ehdr;
	size_t hash_offset;
	size_t hash_size;
	size_t ehdr_size;
	void *data;

	ehdr = (struct elf32_hdr *)fw->data;
	phdrs = (struct elf32_phdr *)(ehdr + 1);

	if (ehdr->e_phnum < 2 || ehdr->e_phnum > PN_XNUM)
		return ERR_PTR(-EINVAL);

	if (phdrs[0].p_type == PT_LOAD)
		return ERR_PTR(-EINVAL);

	if ((phdrs[1].p_flags & QCOM_MDT_TYPE_MASK) != QCOM_MDT_TYPE_HASH)
		return ERR_PTR(-EINVAL);

	ehdr_size = phdrs[0].p_filesz;
	hash_size = phdrs[1].p_filesz;

	data = kmalloc(fw->size, GFP_KERNEL);
	if (!data)
		return ERR_PTR(-ENOMEM);

	/* Is the header and hash already packed */
	if (ehdr_size + hash_size == fw->size)
		hash_offset = phdrs[0].p_filesz;
	else
		hash_offset = phdrs[1].p_offset;

	memcpy(data, fw->data, ehdr_size);
	memcpy(data + hash_offset, fw->data + hash_offset, hash_size);

	*data_len = fw->size;

	return data;
}
EXPORT_SYMBOL_GPL(qcom_mdt_read_metadata);

struct region {
	u64 addr;
	unsigned blk_size;
};

struct pdseg_dma_mem_info {
	struct region *tz_addr;
	int blocks;
	dma_addr_t tz_dma;
	dma_addr_t dma_blk_arr_addr_phys;
	u64 *dma_blk_arr_addr;
	void **pt;
};

static int allocate_dma_mem(struct device *dev,
				struct pdseg_dma_mem_info *pd_dma,
				int max_size)
{
	dma_addr_t dma_tmp = 0;
	int i;

	pd_dma->blocks = DIV_ROUND_UP(max_size, PAGE_SIZE);

	pd_dma->tz_addr = dma_alloc_coherent(dev, sizeof(struct region),
			&pd_dma->tz_dma, GFP_DMA);
	if (!pd_dma->tz_addr) {
		pr_err("Error in dma alloc\n");
		return -ENOMEM;
	}

	pd_dma->dma_blk_arr_addr = dma_alloc_coherent(dev,
			(pd_dma->blocks * sizeof(u64)),
			&pd_dma->dma_blk_arr_addr_phys, GFP_DMA);
	if (!pd_dma->dma_blk_arr_addr) {
		pr_err("Error in dma alloc\n");
		goto free_tz_dma_alloc;
	}
	memcpy(&pd_dma->tz_addr->addr, &pd_dma->dma_blk_arr_addr_phys,
			sizeof(dma_addr_t));

	pd_dma->pt = kzalloc(pd_dma->blocks * sizeof(void *), GFP_KERNEL);
	if (!pd_dma->pt) {
		pr_err("Error in memory alloc\n");
		goto free_dma_blk_arr_alloc;
	}

	for (i = 0; i < pd_dma->blocks; i++) {
		pd_dma->pt[i] = dma_alloc_coherent(dev, PAGE_SIZE,
				&dma_tmp, GFP_DMA);
		if (!pd_dma->pt[i]) {
			pr_err("Error in dma alloc i:%d - blocks:%d\n", i,
							pd_dma->blocks);
			goto free_mem_alloc;
		}
		memcpy(&pd_dma->dma_blk_arr_addr[i], &dma_tmp,
						sizeof(dma_addr_t));
	}
	pd_dma->tz_addr->blk_size = PAGE_SIZE;
	return 0;

free_mem_alloc:
	i = 0;
	while (i < pd_dma->blocks && pd_dma->pt[i]) {
		memcpy(&dma_tmp, &pd_dma->dma_blk_arr_addr[i],
						sizeof(dma_addr_t));
		dma_free_coherent(dev, PAGE_SIZE, pd_dma->pt[i], dma_tmp);
		i++;
	}
	kfree(pd_dma->pt);
free_dma_blk_arr_alloc:
	dma_free_coherent(dev, (pd_dma->blocks * sizeof(u64)),
				pd_dma->dma_blk_arr_addr,
				pd_dma->dma_blk_arr_addr_phys);
free_tz_dma_alloc:
	dma_free_coherent(dev, sizeof(struct region), pd_dma->tz_addr,
							pd_dma->tz_dma);

	return -ENOMEM;
}

static void free_dma_mem(struct device *dev, struct pdseg_dma_mem_info *pd_dma)
{
	int i;
	dma_addr_t dma_tmp = 0;

	for (i = 0; i < pd_dma->blocks; i++) {
		memcpy(&dma_tmp, &pd_dma->dma_blk_arr_addr[i],
				sizeof(dma_addr_t));
		dma_free_coherent(dev, PAGE_SIZE, pd_dma->pt[i],
				dma_tmp);
	}

	dma_free_coherent(dev, (pd_dma->blocks * sizeof(u64)),
					pd_dma->dma_blk_arr_addr,
					pd_dma->dma_blk_arr_addr_phys);

	dma_free_coherent(dev, sizeof(struct region), pd_dma->tz_addr,
			pd_dma->tz_dma);
	kfree(pd_dma->pt);
}

static int memcpy_pdseg_to_dma_blk(const char *fw_name, struct device *dev,
		int ph_no, struct pdseg_dma_mem_info *pd_dma, size_t filesz)
{
	const struct firmware *seg_fw;
	int ret, offset_tmp = 0, tmp = 0;
	size_t size = 0;

	ret = request_firmware(&seg_fw, fw_name, dev);
	if (ret) {
		dev_err(dev, "failed to load %s\n", fw_name);
		return ret;
	}
	size = seg_fw->size < PAGE_SIZE ?
		seg_fw->size : PAGE_SIZE;
	while (tmp < pd_dma->blocks && size) {
		memset_io(pd_dma->pt[tmp], 0,
				PAGE_SIZE);
		memcpy_toio(pd_dma->pt[tmp],
				seg_fw->data + offset_tmp,
				size);
		tmp++;
		offset_tmp += size;
		if ((seg_fw->size - offset_tmp) < PAGE_SIZE)
			size = seg_fw->size - offset_tmp;
	}
	release_firmware(seg_fw);
	ret = qti_scm_pdseg_memcpy_v2(PDSEG_PAS_ID, ph_no, pd_dma->tz_dma,
									tmp);
	if (ret) {
		dev_err(dev, "pd seg memcpy scm failed\n");
		return ret;
	}
	return ret;
}

static bool check_fw_shared(struct device *dev)
{
	const char *fw_name, *parent_fw_name;

	of_property_read_string(dev->of_node, "firmware",
				&fw_name);
	of_property_read_string(dev->parent->of_node, "firmware",
				&parent_fw_name);

	if (!strcmp(fw_name, parent_fw_name))
		return true;

	return false;
}

int get_pd_fw_info(struct device *dev, const struct firmware *fw,
			phys_addr_t mem_phys, size_t mem_size, u8 pd_asid,
			struct qcom_pd_fw_info *fw_info)
{
	const struct elf32_phdr *phdrs;
	const struct elf32_phdr *phdr;
	const struct elf32_hdr *ehdr;
	phys_addr_t mem_reloc;
	phys_addr_t min_addr = PHYS_ADDR_MAX;
	phys_addr_t max_addr = 0;
	ssize_t offset;
	bool relocate = false;
	int ret = 0, i;
	bool fw_shared;

	/* Check FW is shared with parent or not */
	fw_shared = check_fw_shared(dev);

	if (!fw || !mem_phys || !mem_size)
		return -EINVAL;

	ehdr = (struct elf32_hdr *)fw->data;
	phdrs = (struct elf32_phdr *)(ehdr + 1);

	for (i = 0; i < ehdr->e_phnum; i++) {
		phdr = &phdrs[i];

		if (!mdt_phdr_valid(phdr))
			continue;

		/*
		 * While doing PD specific reloading, load only that PD
		 * specific writeable entries. Skip others
		 */
		if (pd_asid && fw_shared && (QCOM_MDT_PF_ASID(phdr->p_flags) !=
					pd_asid || (phdr->p_flags & PF_W) == 0))
			continue;

		if (phdr->p_flags & QCOM_MDT_RELOCATABLE)
			relocate = true;

		if (phdr->p_paddr < min_addr)
			min_addr = phdr->p_paddr;

		if (phdr->p_paddr + phdr->p_memsz > max_addr)
			max_addr = ALIGN(phdr->p_paddr + phdr->p_memsz, SZ_4K);

	}

	if (relocate) {
		/*
		 * The image is relocatable, so offset each segment based on
		 * the lowest segment address.
		 */
		mem_reloc = min_addr;
	} else {
		/*
		 * Image is not relocatable, so offset each segment based on
		 * the allocated physical chunk of memory.
		 */
		mem_reloc = mem_phys;
	}

	for (i = 0; i < ehdr->e_phnum; i++) {
		phdr = &phdrs[i];

		if (!mdt_phdr_valid(phdr))
			continue;

		/*
		 * While doing PD specific reloading, load only that PD
		 * specific writeable entries. Skip others
		 */
		if (pd_asid && fw_shared && (QCOM_MDT_PF_ASID(phdr->p_flags) !=
					pd_asid || (phdr->p_flags & PF_W) == 0))
			continue;

		offset = phdr->p_paddr - mem_reloc;
		if (offset < 0 || offset + phdr->p_memsz > mem_size) {
			dev_err(dev, "segment outside memory range\n");
			ret = -EINVAL;
			break;
		}

		if (!fw_info->paddr) {
			fw_info->paddr = mem_phys + offset;
			fw_info->vaddr = (void *)(uintptr_t)phdr->p_vaddr;
		}

		fw_info->size += phdr->p_memsz;
	}
	return ret;
}
EXPORT_SYMBOL(get_pd_fw_info);

static int __qcom_mdt_load_pd_seg(struct device *dev, const struct firmware *fw,
			   const char *firmware, int pas_id, void *mem_region,
			   phys_addr_t mem_phys, size_t mem_size,
			   phys_addr_t *reloc_base, bool pas_init)
{
	const struct elf32_phdr *phdrs;
	const struct elf32_phdr *phdr;
	const struct elf32_hdr *ehdr;
	const struct firmware *seg_fw;
	phys_addr_t mem_reloc;
	phys_addr_t min_addr = PHYS_ADDR_MAX;
	phys_addr_t max_addr = 0;
	size_t fw_name_len;
	ssize_t offset;
	char *fw_name;
	bool relocate = false;
	void *ptr;
	int ret = 0;
	int i;
	u8 pd_asid;
	int max_size = 0;
	struct pdseg_dma_mem_info pd_dma = {0};

	if (!fw || !mem_region || !mem_phys || !mem_size)
		return -EINVAL;


	pd_asid = qcom_get_pd_asid(dev->of_node);

	ehdr = (struct elf32_hdr *)fw->data;
	phdrs = (struct elf32_phdr *)(ehdr + 1);

	fw_name_len = strlen(firmware);
	if (fw_name_len <= 4)
		return -EINVAL;

	fw_name = kstrdup(firmware, GFP_KERNEL);
	if (!fw_name)
		return -ENOMEM;

	for (i = 0; i < ehdr->e_phnum; i++) {
		phdr = &phdrs[i];

		if (!mdt_phdr_valid(phdr))
			continue;

		/*
		 * While doing PD specific reloading, load only that PD
		 * specific writeable entries. Skip others
		 */
		if (pd_asid && ((QCOM_MDT_PF_ASID(phdr->p_flags) != pd_asid) ||
				((phdr->p_flags & PF_W) == 0)))
			continue;

		if (phdr->p_flags & QCOM_MDT_RELOCATABLE)
			relocate = true;

		if (phdr->p_paddr < min_addr)
			min_addr = phdr->p_paddr;

		if (phdr->p_paddr + phdr->p_memsz > max_addr)
			max_addr = ALIGN(phdr->p_paddr + phdr->p_memsz, SZ_4K);

		if (pd_asid && (max_size < phdr->p_memsz))
			max_size = phdr->p_memsz;
	}

	if (pas_init) {
		ret = allocate_dma_mem(dev, &pd_dma, max_size);
		if (ret)
			goto out;
	}

	if (relocate) {
		if (pas_init) {
			ret = qcom_scm_pas_mem_setup(pas_id, mem_phys,
						     max_addr - min_addr);
			if (ret) {
				dev_err(dev, "unable to setup relocation\n");
				goto out;
			}
		}

		/*
		 * The image is relocatable, so offset each segment based on
		 * the lowest segment address.
		 */
		mem_reloc = min_addr;
	} else {
		/*
		 * Image is not relocatable, so offset each segment based on
		 * the allocated physical chunk of memory.
		 */
		mem_reloc = mem_phys;
	}

	for (i = 0; i < ehdr->e_phnum; i++) {
		phdr = &phdrs[i];

		if (!mdt_phdr_valid(phdr))
			continue;

		/*
		 * While doing PD specific reloading, load only that PD
		 * specific writeable entries. Skip others
		 */
		if (pd_asid && ((QCOM_MDT_PF_ASID(phdr->p_flags) != pd_asid) ||
				((phdr->p_flags & PF_W) == 0)))
			continue;

		offset = phdr->p_paddr - mem_reloc;
		if (offset < 0 || offset + phdr->p_memsz > mem_size) {
			dev_err(dev, "segment outside memory range\n");
			ret = -EINVAL;
			break;
		}

		if (phdr->p_filesz > phdr->p_memsz) {
			dev_err(dev,
				"refusing to load segment %d with p_filesz > p_memsz\n",
				i);
			ret = -EINVAL;
			break;
		}

		if (!pas_init)
			ptr = mem_region + offset;

		if (phdr->p_filesz) {
			/* Firmware not large enough, load split-out segments */
			snprintf(fw_name + fw_name_len - 3, 4, "b%02d", i);

			if (pas_init) {
				ret = memcpy_pdseg_to_dma_blk(fw_name, dev, i,
						&pd_dma, phdr->p_filesz);
				if (ret)
					goto free_dma;
				continue;
			}

			ret = request_firmware_into_buf(&seg_fw, fw_name, dev,
							ptr, phdr->p_filesz);
			if (ret) {
				dev_err(dev, "failed to load %s\n", fw_name);
				break;
			}

			if (seg_fw->size != phdr->p_filesz) {
				dev_err(dev,
					"failed to load segment %d from truncated file %s\n",
					i, fw_name);
				release_firmware(seg_fw);
				ret = -EINVAL;
				break;
			}

			release_firmware(seg_fw);

			if (phdr->p_memsz > phdr->p_filesz)
				memset(ptr + phdr->p_filesz, 0, phdr->p_memsz - phdr->p_filesz);
		}
	}

free_dma:
	if (pas_init)
		free_dma_mem(dev, &pd_dma);

	if (reloc_base)
		*reloc_base = mem_reloc;

out:
	kfree(fw_name);

	return ret;
}

static int __qcom_mdt_load(struct device *dev, const struct firmware *fw,
			   const char *firmware, int pas_id, void *mem_region,
			   phys_addr_t mem_phys, size_t mem_size,
			   phys_addr_t *reloc_base, bool pas_init)
{
	const struct elf32_phdr *phdrs;
	const struct elf32_phdr *phdr;
	const struct elf32_hdr *ehdr;
	const struct firmware *seg_fw;
	phys_addr_t mem_reloc;
	phys_addr_t min_addr = PHYS_ADDR_MAX;
	phys_addr_t max_addr = 0;
	size_t metadata_len;
	size_t fw_name_len;
	ssize_t offset;
	void *metadata;
	char *fw_name;
	bool relocate = false;
	void *ptr;
	int ret = 0;
	int i;

	if (!fw || !mem_region || !mem_phys || !mem_size)
		return -EINVAL;

	ehdr = (struct elf32_hdr *)fw->data;
	phdrs = (struct elf32_phdr *)(ehdr + 1);

	fw_name_len = strlen(firmware);
	if (fw_name_len <= 4)
		return -EINVAL;

	fw_name = kstrdup(firmware, GFP_KERNEL);
	if (!fw_name)
		return -ENOMEM;

	if (pas_init) {
		metadata = qcom_mdt_read_metadata(fw, &metadata_len);
		if (IS_ERR(metadata)) {
			ret = PTR_ERR(metadata);
			goto out;
		}

		ret = qcom_scm_pas_init_image(pas_id, metadata, metadata_len);
		kfree(metadata);
		if (ret) {
			dev_err(dev, "invalid firmware metadata\n");
			goto out;
		}
	}

	for (i = 0; i < ehdr->e_phnum; i++) {
		phdr = &phdrs[i];

		if (!mdt_phdr_valid(phdr))
			continue;

		if (phdr->p_flags & QCOM_MDT_RELOCATABLE)
			relocate = true;

		if (phdr->p_paddr < min_addr)
			min_addr = phdr->p_paddr;

		if (phdr->p_paddr + phdr->p_memsz > max_addr)
			max_addr = ALIGN(phdr->p_paddr + phdr->p_memsz, SZ_4K);
	}

	if (relocate) {
		if (pas_init) {
			ret = qcom_scm_pas_mem_setup(pas_id, mem_phys,
						     max_addr - min_addr);
			if (ret) {
				dev_err(dev, "unable to setup relocation\n");
				goto out;
			}
		}

		/*
		 * The image is relocatable, so offset each segment based on
		 * the lowest segment address.
		 */
		mem_reloc = min_addr;
	} else {
		/*
		 * Image is not relocatable, so offset each segment based on
		 * the allocated physical chunk of memory.
		 */
		mem_reloc = mem_phys;
	}

	for (i = 0; i < ehdr->e_phnum; i++) {
		phdr = &phdrs[i];

		if (!mdt_phdr_valid(phdr))
			continue;

		offset = phdr->p_paddr - mem_reloc;
		if (offset < 0 || offset + phdr->p_memsz > mem_size) {
			dev_err(dev, "segment outside memory range\n");
			ret = -EINVAL;
			break;
		}

		if (phdr->p_filesz > phdr->p_memsz) {
			dev_err(dev,
				"refusing to load segment %d with p_filesz > p_memsz\n",
				i);
			ret = -EINVAL;
			break;
		}

		ptr = mem_region + offset;

		if (phdr->p_filesz && phdr->p_offset < fw->size) {
			/* Firmware is large enough to be non-split */
			if (phdr->p_offset + phdr->p_filesz > fw->size) {
				dev_err(dev,
					"failed to load segment %d from truncated file %s\n",
					i, firmware);
				ret = -EINVAL;
				break;
			}
			memcpy(ptr, fw->data + phdr->p_offset, phdr->p_filesz);
		} else if (phdr->p_filesz) {
			/* Firmware not large enough, load split-out segments */
			sprintf(fw_name + fw_name_len - 3, "b%02d", i);

			ret = request_firmware_into_buf(&seg_fw, fw_name, dev,
							ptr, phdr->p_filesz);
			if (ret) {
				dev_err(dev, "failed to load %s\n", fw_name);
				break;
			}

			if (seg_fw->size != phdr->p_filesz) {
				dev_err(dev,
					"failed to load segment %d from truncated file %s\n",
					i, fw_name);
				release_firmware(seg_fw);
				ret = -EINVAL;
				break;
			}

			release_firmware(seg_fw);
		}

		if (phdr->p_memsz > phdr->p_filesz)
			memset(ptr + phdr->p_filesz, 0,
					phdr->p_memsz - phdr->p_filesz);
	}

	if (reloc_base)
		*reloc_base = mem_reloc;

out:
	kfree(fw_name);

	return ret;
}

/**
 * qcom_mdt_load() - load the firmware which header is loaded as fw
 * @dev:	device handle to associate resources with
 * @fw:		firmware object for the mdt file
 * @firmware:	name of the firmware, for construction of segment file names
 * @pas_id:	PAS identifier
 * @mem_region:	allocated memory region to load firmware into
 * @mem_phys:	physical address of allocated memory region
 * @mem_size:	size of the allocated memory region
 * @reloc_base:	adjusted physical address after relocation
 *
 * Returns 0 on success, negative errno otherwise.
 */
int qcom_mdt_load(struct device *dev, const struct firmware *fw,
		  const char *firmware, int pas_id, void *mem_region,
		  phys_addr_t mem_phys, size_t mem_size,
		  phys_addr_t *reloc_base)
{
	return __qcom_mdt_load(dev, fw, firmware, pas_id, mem_region, mem_phys,
			       mem_size, reloc_base, true);
}
EXPORT_SYMBOL_GPL(qcom_mdt_load);

/**
 * qcom_mdt_load_no_init() - load the firmware which header is loaded as fw
 * @dev:	device handle to associate resources with
 * @fw:		firmware object for the mdt file
 * @firmware:	name of the firmware, for construction of segment file names
 * @pas_id:	PAS identifier
 * @mem_region:	allocated memory region to load firmware into
 * @mem_phys:	physical address of allocated memory region
 * @mem_size:	size of the allocated memory region
 * @reloc_base:	adjusted physical address after relocation
 *
 * Returns 0 on success, negative errno otherwise.
 */
int qcom_mdt_load_no_init(struct device *dev, const struct firmware *fw,
			  const char *firmware, int pas_id,
			  void *mem_region, phys_addr_t mem_phys,
			  size_t mem_size, phys_addr_t *reloc_base)
{
	return __qcom_mdt_load(dev, fw, firmware, pas_id, mem_region, mem_phys,
			       mem_size, reloc_base, false);
}
EXPORT_SYMBOL_GPL(qcom_mdt_load_no_init);

/**
 * qcom_mdt_load_pd_seg() - load the firmware which header is loaded as fw
 * @dev:	device handle to associate resources with
 * @fw:		firmware object for the mdt file
 * @firmware:	name of the firmware, for construction of segment file names
 * @pas_id:	PAS identifier
 * @mem_region:	allocated memory region to load firmware into
 * @mem_phys:	physical address of allocated memory region
 * @mem_size:	size of the allocated memory region
 * @reloc_base:	adjusted physical address after relocation
 *
 * Returns 0 on success, negative errno otherwise.
 */
int qcom_mdt_load_pd_seg(struct device *dev, const struct firmware *fw,
		  const char *firmware, int pas_id, void *mem_region,
		  phys_addr_t mem_phys, size_t mem_size,
		  phys_addr_t *reloc_base)
{
	return __qcom_mdt_load_pd_seg(dev, fw, firmware, pas_id, mem_region,
				mem_phys, mem_size, reloc_base, true);
}
EXPORT_SYMBOL_GPL(qcom_mdt_load_pd_seg);

/**
 * qcom_mdt_load_no_init() - load the firmware which header is loaded as fw
 * @dev:	device handle to associate resources with
 * @fw:		firmware object for the mdt file
 * @firmware:	name of the firmware, for construction of segment file names
 * @pas_id:	PAS identifier
 * @mem_region:	allocated memory region to load firmware into
 * @mem_phys:	physical address of allocated memory region
 * @mem_size:	size of the allocated memory region
 * @reloc_base:	adjusted physical address after relocation
 *
 * Returns 0 on success, negative errno otherwise.
 */
int qcom_mdt_load_pd_seg_no_init(struct device *dev, const struct firmware *fw,
			  const char *firmware, int pas_id,
			  void *mem_region, phys_addr_t mem_phys,
			  size_t mem_size, phys_addr_t *reloc_base)
{
	return __qcom_mdt_load_pd_seg(dev, fw, firmware, pas_id, mem_region,
				mem_phys, mem_size, reloc_base, false);
}
EXPORT_SYMBOL_GPL(qcom_mdt_load_pd_seg_no_init);

MODULE_DESCRIPTION("Firmware parser for Qualcomm MDT format");
MODULE_LICENSE("GPL v2");
