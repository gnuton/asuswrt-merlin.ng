lzma_compress-objs := LzFind.o LzmaEnc.o
lzma_decompress-objs := LzmaDec.o

obj-$(CONFIG_LZMA_COMPRESS) += lzma_compress.o
obj-$(CONFIG_LZMA_DECOMPRESS) += lzma_decompress.o

EXTRA_CFLAGS += -Iinclude/linux -Iinclude/linux/lzma -include types.h
