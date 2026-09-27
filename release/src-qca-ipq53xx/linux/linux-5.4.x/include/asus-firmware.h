#ifndef _ASUS_FIRMWARE_H_
#define _ASUS_FIRMWARE_H_

#define IH_NMLEN		32	/* Image Name Length		*/

#if defined(CONFIG_MTD_UBI)
/* NAND page size larger than 512Bytes, block size larger than 64K */
#define MTD_BOOT_PART_SIZE	0x3E0000
#define MTD_CONFIG_PART_SIZE	0x20000
#define MTD_FACTORY_PART_SIZE	0x20000
#else
#define MTD_BOOT_PART_SIZE	0x40000
#define	MTD_CONFIG_PART_SIZE	0x10000
#define	MTD_FACTORY_PART_SIZE	0x10000
#endif

#define	MTD_KERN_PART_SIZE	0	/* fix-up at run-time */
#define	MTD_ROOTFS_PART_SIZE	0	/* fix-up at run-time */
#define	MTD_JFFS2_PART_SIZE	0x80000

#define MAX_STRING 12
#define MAX_VER 4

/* If hw[i].kernel == ROOTFS_OFFSET_MAGIC,
 * rootfilesystem offset (uImage header size + kernel size)
 * can be calculated by following equation:
 * (hw[i].minor << 16) | (hw[i+1].major << 8) | (hw[i+1].minor)
 */
#define ROOTFS_OFFSET_MAGIC	0xA9	/* Occupy two version_t		*/

typedef struct {
	uint8_t major;
	uint8_t minor;
} version_t;

typedef struct {
	version_t kernel;
	version_t fs;
	char	  productid[MAX_STRING];
	version_t hw[MAX_VER*2];
} TAIL;

typedef struct image_header {
	uint32_t	ih_magic;	/* Image Header Magic Number	*/
	uint32_t	ih_hcrc;	/* Image Header CRC Checksum	*/
	uint32_t	ih_time;	/* Image Creation Timestamp	*/
	uint32_t	ih_size;	/* Image Data Size		*/
	uint32_t	ih_load;	/* Data	 Load  Address		*/
	uint32_t	ih_ep;		/* Entry Point Address		*/
	uint32_t	ih_dcrc;	/* Image Data CRC Checksum	*/
	uint8_t		ih_os;		/* Operating System		*/
	uint8_t		ih_arch;	/* CPU architecture		*/
	uint8_t		ih_type;	/* Image Type			*/
	uint8_t		ih_comp;	/* Compression Type		*/
	union {
		uint8_t		ih_name[IH_NMLEN];	/* Image Name		*/
		TAIL		tail;		/* ASUS firmware infomation	*/
	} u;
} image_header_t;

#endif	/* _ASUS_FIRMWARE_H_ */

