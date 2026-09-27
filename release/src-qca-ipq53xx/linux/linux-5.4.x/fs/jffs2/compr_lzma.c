/*
 * JFFS2 -- Journalling Flash File System, Version 2.
 *
 * For licensing information, see the file 'LICENCE' in this directory.
 *
 * JFFS2 wrapper to the LZMA C SDK
 *
 */

#include <linux/lzma.h>
#include "compr.h"

#ifdef __KERNEL__
	static DEFINE_MUTEX(deflate_mutex);
#endif

CLzmaEncHandle *p;
Byte propsEncoded[LZMA_PROPS_SIZE];
SizeT propsSize = sizeof(propsEncoded);

STATIC void lzma_free_workspace(void)
{
	LzmaEnc_Destroy(p, &lzma_alloc, &lzma_alloc);
}

STATIC int INIT lzma_alloc_workspace(CLzmaEncProps *props)
{
	if ((p = (CLzmaEncHandle *)LzmaEnc_Create(&lzma_alloc)) == NULL)
	{
		PRINT_ERROR("Failed to allocate lzma deflate workspace\n");
		return -ENOMEM;
	}

	if (LzmaEnc_SetProps(p, props) != SZ_OK)
	{
		lzma_free_workspace();
		return -1;
	}
	
	if (LzmaEnc_WriteProperties(p, propsEncoded, &propsSize) != SZ_OK)
	{
		lzma_free_workspace();
		return -1;
	}

        return 0;
}

STATIC int jffs2_lzma_compress(unsigned char *data_in, unsigned char *cpage_out,
			      uint32_t *sourcelen, uint32_t *dstlen)
{
	SizeT compress_size = (SizeT)(*dstlen);
	int ret;

	#ifdef __KERNEL__
		mutex_lock(&deflate_mutex);
	#endif

	ret = LzmaEnc_MemEncode(p, cpage_out, &compress_size, data_in, *sourcelen,
		0, NULL, &lzma_alloc, &lzma_alloc);

	#ifdef __KERNEL__
		mutex_unlock(&deflate_mutex);
	#endif

	if (ret != SZ_OK)
		return -1;

	*dstlen = (uint32_t)compress_size;

	return 0;
}

STATIC int jffs2_lzma_decompress(unsigned char *data_in, unsigned char *cpage_out,
				 uint32_t srclen, uint32_t destlen)
{
	int ret;
	SizeT dl = (SizeT)destlen;
	SizeT sl = (SizeT)srclen;
	ELzmaStatus status;
	
	ret = LzmaDecode(cpage_out, &dl, data_in, &sl, propsEncoded,
		propsSize, LZMA_FINISH_ANY, &status, &lzma_alloc);

	if (ret != SZ_OK || status == LZMA_STATUS_NOT_FINISHED || dl != (SizeT)destlen)
		return -1;

	return 0;
}

static struct jffs2_compressor jffs2_lzma_comp = {
	.priority = JFFS2_LZMA_PRIORITY,
	.name = "lzma",
	.compr = JFFS2_COMPR_LZMA,
	.compress = &jffs2_lzma_compress,
	.decompress = &jffs2_lzma_decompress,
	.disabled = 0,
};

int INIT jffs2_lzma_init(void)
{
        int ret;
	CLzmaEncProps props;
	LzmaEncProps_Init(&props);

        props.dictSize = LZMA_BEST_DICT(0x2000);
        props.level = LZMA_BEST_LEVEL;
        props.lc = LZMA_BEST_LC;
        props.lp = LZMA_BEST_LP;
        props.pb = LZMA_BEST_PB;
        props.fb = LZMA_BEST_FB;

	ret = lzma_alloc_workspace(&props);
        if (ret < 0)
                return ret;

	ret = jffs2_register_compressor(&jffs2_lzma_comp);
	if (ret)
		lzma_free_workspace();
	
        return ret;
}

void jffs2_lzma_exit(void)
{
	jffs2_unregister_compressor(&jffs2_lzma_comp);
	lzma_free_workspace();
}
