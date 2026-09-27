// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2016, The Linux Foundation. All rights reserved.
 */
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/bitops.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/module.h>
#include <linux/mtd/rawnand.h>
#include <linux/mtd/partitions.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/dma/qcom_bam_dma.h>

/* NANDc reg offsets */
#define	NAND_FLASH_CMD			0x00
#define	NAND_ADDR0			0x04
#define	NAND_ADDR1			0x08
#define	NAND_FLASH_CHIP_SELECT		0x0c
#define	NAND_EXEC_CMD			0x10
#define	NAND_FLASH_STATUS		0x14
#define	NAND_BUFFER_STATUS		0x18
#define	NAND_DEV0_CFG0			0x20
#define	NAND_DEV0_CFG1			0x24
#define	NAND_DEV0_ECC_CFG		0x28
#define	NAND_AUTO_STATUS_EN		0x2c
#define	NAND_DEV1_CFG0			0x30
#define	NAND_DEV1_CFG1			0x34
#define	NAND_READ_ID			0x40
#define	NAND_READ_STATUS		0x44
#define	NAND_DEV_CMD0			0xa0
#define	NAND_DEV_CMD1			0xa4
#define	NAND_DEV_CMD2			0xa8
#define	NAND_DEV_CMD_VLD		0xac
#define	NAND_DEV_CMD7			0xb0
#define	NAND_DEV_CMD8			0xb4
#define	NAND_DEV_CMD9			0xb8
#define	NAND_FLASH_SPI_CFG		0xc0
#define	NAND_SPI_NUM_ADDR_CYCLES	0xc4
#define	NAND_SPI_BUSY_CHECK_WAIT_CNT	0xc8
#define	NAND_DEV_CMD3			0xd0
#define	NAND_DEV_CMD4			0xd4
#define	NAND_DEV_CMD5			0xd8
#define	NAND_DEV_CMD6			0xdc
#define	SFLASHC_BURST_CFG		0xe0
#define	NAND_ERASED_CW_DETECT_CFG	0xe8
#define	NAND_ERASED_CW_DETECT_STATUS	0xec
#define	NAND_EBI2_ECC_BUF_CFG		0xf0
#define	FLASH_BUF_ACC			0x100
#define	NAND_CTRL			0xf00
#define	NAND_VERSION			0xf08
#define	NAND_READ_LOCATION_0		0xf20
#define	NAND_READ_LOCATION_1		0xf24
#define	NAND_READ_LOCATION_2		0xf28
#define	NAND_READ_LOCATION_3		0xf2c
#define	NAND_READ_LOCATION_LAST_CW_0	0xf40
#define	NAND_READ_LOCATION_LAST_CW_1	0xf44
#define	NAND_READ_LOCATION_LAST_CW_2	0xf48
#define	NAND_READ_LOCATION_LAST_CW_3	0xf4c
#define	NAND_QSPI_MSTR_CONFIG		0xf60
#define	NAND_FLASH_FEATURES		0xf64

/* dummy register offsets, used by write_reg_dma */
#define	NAND_DEV_CMD1_RESTORE		0xdead
#define	NAND_DEV_CMD_VLD_RESTORE	0xbeef

#define SW_BAD_BLOCK_INDICATION 0x33

/* NAND_FLASH_CMD bits */
#define	PAGE_ACC			BIT(4)
#define	LAST_PAGE			BIT(5)

/* NAND_FLASH_CHIP_SELECT bits */
#define	NAND_DEV_SEL			0
#define	DM_EN				BIT(2)

/* NAND_FLASH_STATUS bits */
#define	FS_OP_ERR			BIT(4)
#define	FS_READY_BSY_N			BIT(5)
#define	FS_MPU_ERR			BIT(8)
#define	FS_DEVICE_STS_ERR		BIT(16)
#define	FS_DEVICE_WP			BIT(23)
#define	FS_TIMEOUT_ERR			BIT(6)
#define	FLASH_ERROR			(FS_OP_ERR | FS_MPU_ERR | FS_TIMEOUT_ERR)

/* NAND_BUFFER_STATUS bits */
#define	BS_UNCORRECTABLE_BIT		BIT(8)
#define	BS_CORRECTABLE_ERR_MSK		0x1f

/* NAND_DEVn_CFG0 bits */
#define	DISABLE_STATUS_AFTER_WRITE	4
#define	CW_PER_PAGE			6
#define	UD_SIZE_BYTES			9
#define	ECC_PARITY_SIZE_BYTES_RS	19
#define	SPARE_SIZE_BYTES		23
#define	NUM_ADDR_CYCLES			27
#define	STATUS_BFR_READ			30
#define	SET_RD_MODE_AFTER_STATUS	31

/* NAND_DEVn_CFG0 bits */
#define	DEV0_CFG1_ECC_DISABLE		0
#define	WIDE_FLASH			1
#define	NAND_RECOVERY_CYCLES		2
#define	CS_ACTIVE_BSY			5
#define	BAD_BLOCK_BYTE_NUM		6
#define	BAD_BLOCK_IN_SPARE_AREA		16
#define	WR_RD_BSY_GAP			17
#define	ENABLE_BCH_ECC			27

/* NAND_DEV0_ECC_CFG bits */
#define	ECC_CFG_ECC_DISABLE		0
#define	ECC_SW_RESET			1
#define	ECC_MODE			4
#define	ECC_PARITY_SIZE_BYTES_BCH	8
#define	ECC_NUM_DATA_BYTES		16
#define	ECC_FORCE_CLK_OPEN		30

/* NAND_DEV_CMD1 bits */
#define	READ_ADDR			0

/* NAND_DEV_CMD_VLD bits */
#define	READ_START_VLD			BIT(0)
#define	READ_STOP_VLD			BIT(1)
#define	WRITE_START_VLD			BIT(2)
#define	ERASE_START_VLD			BIT(3)
#define	SEQ_READ_START_VLD		BIT(4)

/* NAND_EBI2_ECC_BUF_CFG bits */
#define	NUM_STEPS			0

/* NAND_ERASED_CW_DETECT_CFG bits */
#define	ERASED_CW_ECC_MASK		1
#define	AUTO_DETECT_RES			0
#define	MASK_ECC			(1 << ERASED_CW_ECC_MASK)
#define	RESET_ERASED_DET		(1 << AUTO_DETECT_RES)
#define	ACTIVE_ERASED_DET		(0 << AUTO_DETECT_RES)
#define	CLR_ERASED_PAGE_DET		(RESET_ERASED_DET | MASK_ECC)
#define	SET_ERASED_PAGE_DET		(ACTIVE_ERASED_DET | MASK_ECC)

/* NAND_ERASED_CW_DETECT_STATUS bits */
#define	PAGE_ALL_ERASED			BIT(7)
#define	CODEWORD_ALL_ERASED		BIT(6)
#define	PAGE_ERASED			BIT(5)
#define	CODEWORD_ERASED			BIT(4)
#define	ERASED_PAGE			(PAGE_ALL_ERASED | PAGE_ERASED)
#define	ERASED_CW			(CODEWORD_ALL_ERASED | CODEWORD_ERASED)

/* NAND_READ_LOCATION_n bits */
#define READ_LOCATION_OFFSET		0
#define READ_LOCATION_SIZE		16
#define READ_LOCATION_LAST		31

/* Version Mask */
#define	NAND_VERSION_MAJOR_MASK		0xf0000000
#define	NAND_VERSION_MAJOR_SHIFT	28
#define	NAND_VERSION_MINOR_MASK		0x0fff0000
#define	NAND_VERSION_MINOR_SHIFT	16

/* NAND OP_CMDs */
#define	OP_PAGE_READ			0x2
#define	OP_PAGE_READ_WITH_ECC		0x3
#define	OP_PAGE_READ_WITH_ECC_SPARE	0x4
#define	OP_PROGRAM_PAGE			0x6
#define	OP_PAGE_PROGRAM_WITH_ECC	0x7
#define	OP_PROGRAM_PAGE_SPARE		0x9
#define	OP_BLOCK_ERASE			0xa
#define	OP_FETCH_ID			0xb
#define	OP_RESET_DEVICE			0xd
#define	ACC_FEATURE			0xe

/* Default Value for NAND_DEV_CMD_VLD */
#define NAND_DEV_CMD_VLD_VAL		(READ_START_VLD | WRITE_START_VLD | \
					 ERASE_START_VLD | SEQ_READ_START_VLD)

/* NAND_CTRL bits */
#define	BAM_MODE_EN			BIT(0)

/*
 * the NAND controller performs reads/writes with ECC in 516 byte chunks.
 * the driver calls the chunks 'step' or 'codeword' interchangeably
 */
#define	NANDC_STEP_SIZE			512

/*
 * the largest page size we support is 8K, this will have 16 steps/codewords
 * of 512 bytes each
 */
#define	MAX_NUM_STEPS			(SZ_8K / NANDC_STEP_SIZE)

/* we read at most 3 registers per codeword scan */
#define	MAX_REG_RD			(3 * MAX_NUM_STEPS)

/* ECC modes supported by the controller */
#define	ECC_NONE	BIT(0)
#define	ECC_RS_4BIT	BIT(1)
#define	ECC_BCH_4BIT	BIT(2)
#define	ECC_BCH_8BIT	BIT(3)

/* QSPI NAND config reg bits */
#define	LOAD_CLK_CNTR_INIT_EN	(1 << 28)
#define	CLK_CNTR_INIT_VAL_VEC	0x924
#define	FEA_STATUS_DEV_ADDR	0xc0
#define	SPI_CFG	(1 << 0)

/* CMD register value for qspi nand */
#define	CMD0_VAL	0x1080D8D8
#define	CMD1_VAL	0xF00F3000
#define	CMD2_VAL	0xF0FF709F
#define	CMD3_VAL	0x3F310015
#define	CMD3_MASK	0xfff0ffff
#define	CMD7_VAL	0x04061F0F
#define	CMD_VLD_VAL	0xd
#define	SPI_NUM_ADDR	0xDA4DB
#define	WAIT_CNT	0x10

/*
 * SPI Nand flash device ID's
 */
#define	SPI_FLASH_MICRON_ID		0x2c
#define	SPI_FLASH_ESMT_DEVICE_ID	0x11
#define	SPI_FLASH_WINBOND_ID		0xef
#define	SPI_WINBOND_DEVICE_1		0xba
#define	SPI_FLASH_GIGA_ID		0xc8

/*
 * Serial NAND flash commands
 */
#define	NAND_CMD_READID_SERIAL		0x9F
#define	NAND_CMD_ERASE_SERIAL		0xd8
#define	NAND_CMD_SET_FEATURE_SERIAL	0x1F
#define	NAND_CMD_GET_FEATURE_SERIAL	0x0F
#define	SPI_FLASH_FEATURE_REG		0xB0

/*
 * Serial NAND flash status register bits
 */
#define	SPI_FLASH_QUAD_MODE		0x1
#define	SPI_NAND_BUF_BIT(x)		(1 << x)

/* QSPI NAND CMD reg bits value */
#define	SPI_WP		(1 << 28)
#define	SPI_HOLD	(1 << 27)
#define	SPI_TRANSFER_MODE_x1	(1 << 29)
#define	SPI_TRANSFER_MODE_x4	(3 << 29)
#define	QPIC_SET_FEATURE	(1 << 31)
#define QPIC_v2_0	0x2
#define FEEDBACK_CLK_EN	(1 << 4)
#define MAX_TRAINING_BLK	8
#define TOTAL_NUM_PHASE	7
#define	AUTO_STS_VAL	0x000B000B
#define	PAGE_SCOPE_READ	(1 << 23)
#define	MAX_STATUS_REG	12
#define IO_MACRO_50_MHZ	50000000

#define nandc_set_read_loc_first(nandc, reg, cw_offset, read_size, is_last_read_loc)	\
nandc_set_reg(nandc, reg,			\
	      ((cw_offset) << READ_LOCATION_OFFSET) |		\
	      ((read_size) << READ_LOCATION_SIZE) |			\
	      ((is_last_read_loc) << READ_LOCATION_LAST))

#define nandc_set_read_loc_last(nandc, reg, cw_offset, read_size, is_last_read_loc)	\
nandc_set_reg(nandc, reg,			\
	      ((cw_offset) << READ_LOCATION_OFFSET) |		\
	      ((read_size) << READ_LOCATION_SIZE) |			\
	      ((is_last_read_loc) << READ_LOCATION_LAST))

/*
 * Returns the actual register address for all NAND_DEV_ registers
 * (i.e. NAND_DEV_CMD0, NAND_DEV_CMD1, NAND_DEV_CMD2 and NAND_DEV_CMD_VLD)
 */
#define dev_cmd_reg_addr(nandc, reg) ((nandc)->props->dev_cmd_reg_start + (reg))

/* Returns the NAND register physical address */
#define nandc_reg_phys(chip, offset) ((chip)->base_phys + (offset))

/* Returns the dma address for reg read buffer */
#define reg_buf_dma_addr(chip, vaddr) \
	((chip)->reg_read_dma + \
	((uint8_t *)(vaddr) - (uint8_t *)(chip)->reg_read_buf))

#define QPIC_PER_CW_CMD_ELEMENTS	32
#define QPIC_PER_CW_CMD_SGL		32
#define QPIC_PER_CW_DATA_SGL		8
#define	QPIC_PER_CW_STS_SGL		8

#define QPIC_NAND_COMPLETION_TIMEOUT	msecs_to_jiffies(6000)

/*
 * Flags used in DMA descriptor preparation helper functions
 * (i.e. read_reg_dma/write_reg_dma/read_data_dma/write_data_dma)
 */
/* Don't set the EOT in current tx BAM sgl */
#define NAND_BAM_NO_EOT			BIT(0)
/* Set the NWD flag in current BAM sgl */
#define NAND_BAM_NWD			BIT(1)
/* Finish writing in the current BAM sgl and start writing in another BAM sgl */
#define NAND_BAM_NEXT_SGL		BIT(2)
/*
 * Erased codeword status is being used two times in single transfer so this
 * flag will determine the current value of erased codeword status register
 */
#define NAND_ERASED_CW_SET		BIT(4)

#define NAND_MID_WINBOND		0xEF

/*
 * An array holding the fixed pattern
 */
static const u32 qspi_training_block_64[] = {
	0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F,
	0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F,
	0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F,
	0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F,
};

struct nand_flash_dev qspinand_flash_ids_2k[] = {
	{"MX35UF4GE4AD-Z4I SPI NAND 1G 1.8V",
			{ .id = {0xc2, 0xb7} },
		SZ_2K, SZ_256, SZ_128K, 0, 2, 128, NAND_ECC_INFO(8, SZ_512), 0},
	{NULL}
};

/*
 * This data type corresponds to the BAM transaction which will be used for all
 * NAND transfers.
 * @bam_ce - the array of BAM command elements
 * @cmd_sgl - sgl for NAND BAM command pipe
 * @data_sgl - sgl for NAND BAM consumer/producer pipe
 * @bam_ce_pos - the index in bam_ce which is available for next sgl
 * @bam_ce_start - the index in bam_ce which marks the start position ce
 *		   for current sgl. It will be used for size calculation
 *		   for current sgl
 * @cmd_sgl_pos - current index in command sgl.
 * @cmd_sgl_start - start index in command sgl.
 * @tx_sgl_pos - current index in data sgl for tx.
 * @tx_sgl_start - start index in data sgl for tx.
 * @rx_sgl_pos - current index in data sgl for rx.
 * @rx_sgl_start - start index in data sgl for rx.
 * @wait_second_completion - wait for second DMA desc completion before making
 *			     the NAND transfer completion.
 * @txn_done - completion for NAND transfer.
 * @last_data_desc - last DMA desc in data channel (tx/rx).
 * @last_cmd_desc - last DMA desc in command channel.
 */
struct bam_transaction {
	struct bam_cmd_element *bam_ce;
	struct scatterlist *cmd_sgl;
	struct scatterlist *data_sgl;
	struct scatterlist *sts_sgl;
	u32 bam_ce_pos;
	u32 bam_ce_start;
	u32 cmd_sgl_pos;
	u32 cmd_sgl_start;
	u32 tx_sgl_pos;
	u32 tx_sgl_start;
	u32 rx_sgl_pos;
	u32 rx_sgl_start;
	u32 sts_sgl_pos;
	u32 sts_sgl_start;
	bool wait_second_completion;
	struct completion txn_done;
	struct dma_async_tx_descriptor *last_data_desc;
	struct dma_async_tx_descriptor *last_cmd_desc;
};

/*
 * This data type corresponds to the nand dma descriptor
 * @list - list for desc_info
 * @dir - DMA transfer direction
 * @adm_sgl - sgl which will be used for single sgl dma descriptor. Only used by
 *	      ADM
 * @bam_sgl - sgl which will be used for dma descriptor. Only used by BAM
 * @sgl_cnt - number of SGL in bam_sgl. Only used by BAM
 * @dma_desc - low level DMA engine descriptor
 */
struct desc_info {
	struct list_head node;

	enum dma_data_direction dir;
	union {
		struct scatterlist adm_sgl;
		struct {
			struct scatterlist *bam_sgl;
			int sgl_cnt;
		};
	};
	struct dma_async_tx_descriptor *dma_desc;
};

/*
 * holds the current register values that we want to write. acts as a contiguous
 * chunk of memory which we use to write the controller registers through DMA.
 */
struct nandc_regs {
	__le32 cmd;
	__le32 addr0;
	__le32 addr1;
	__le32 chip_sel;
	__le32 exec;

	__le32 cfg0;
	__le32 cfg1;
	__le32 ecc_bch_cfg;

	__le32 clrflashstatus;
	__le32 clrreadstatus;

	__le32 cmd1;
	__le32 vld;

	__le32 orig_cmd1;
	__le32 orig_vld;

	__le32 ecc_buf_cfg;
	__le32 read_location0;
	__le32 read_location1;
	__le32 read_location2;
	__le32 read_location3;
	__le32 read_location_last0;
	__le32 read_location_last1;
	__le32 read_location_last2;
	__le32 read_location_last3;
	__le32 flash_feature;
	__le32 spi_cfg;
	__le32 num_addr_cycle;
	__le32 busy_wait_cnt;
	__le32 mstr_cfg;

	__le32 erased_cw_detect_cfg_clr;
	__le32 erased_cw_detect_cfg_set;

	__le32 auto_sts_en;
};

/*
 * NAND controller data struct
 *
 * @controller:			base controller structure
 * @host_list:			list containing all the chips attached to the
 *				controller
 * @dev:			parent device
 * @base:			MMIO base
 * @base_phys:			physical base address of controller registers
 * @base_dma:			dma base address of controller registers
 * @core_clk:			controller clock
 * @aon_clk:			another controller clock
 *
 * @chan:			dma channel
 * @cmd_crci:			ADM DMA CRCI for command flow control
 * @data_crci:			ADM DMA CRCI for data flow control
 * @desc_list:			DMA descriptor list (list of desc_infos)
 *
 * @data_buffer:		our local DMA buffer for page read/writes,
 *				used when we can't use the buffer provided
 *				by upper layers directly
 * @boot_layout:		flag to tell whether current layout is boot
 *				layout
 * @buf_size/count/start:	markers for chip->legacy.read_buf/write_buf
 *				functions
 * @reg_read_buf:		local buffer for reading back registers via DMA
 * @reg_read_dma:		contains dma address for register read buffer
 * @reg_read_pos:		marker for data read in reg_read_buf
 *
 * @regs:			a contiguous chunk of memory for DMA register
 *				writes. contains the register values to be
 *				written to controller
 * @cmd1/vld:			some fixed controller register values
 * @props:			properties of current NAND controller,
 *				initialized via DT match data
 * @max_cwperpage:		maximum QPIC codewords required. calculated
 *				from all connected NAND devices pagesize
 */
struct qcom_nand_controller {
	struct nand_controller controller;
	struct list_head host_list;

	struct device *dev;

	void __iomem *base;
	phys_addr_t base_phys;
	dma_addr_t base_dma;

	struct clk *core_clk;
	struct clk *aon_clk;
	struct clk *iomacro_clk;

	union {
		/* will be used only by QPIC for BAM DMA */
		struct {
			struct dma_chan *tx_chan;
			struct dma_chan *rx_chan;
			struct dma_chan *cmd_chan;
			struct dma_chan *sts_chan;
		};

		/* will be used only by EBI2 for ADM DMA */
		struct {
			struct dma_chan *chan;
			unsigned int cmd_crci;
			unsigned int data_crci;
		};
	};

	struct list_head desc_list;
	struct bam_transaction *bam_txn;

	u8		*data_buffer;
	bool		boot_layout;
	int		buf_size;
	int		buf_count;
	int		buf_start;
	unsigned int	max_cwperpage;

	__le32 *reg_read_buf;
	dma_addr_t reg_read_dma;
	int reg_read_pos;

	struct nandc_regs *regs;

	u32 cmd1, vld;
	const struct qcom_nandc_props *props;

	__le32 *status_buf;
	int sts_buf_size;
};

/*
 * NAND chip structure
 *
 * @chip:			base NAND chip structure
 * @node:			list node to add itself to host_list in
 *				qcom_nand_controller
 *
 * @cs:				chip select value for this chip
 * @cw_size:			the number of bytes in a single step/codeword
 *				of a page, consisting of all data, ecc, spare
 *				and reserved bytes
 * @cw_data:			the number of bytes within a codeword protected
 *				by ECC
 * @use_ecc:			request the controller to use ECC for the
 *				upcoming read/write
 * @bch_enabled:		flag to tell whether BCH ECC mode is used
 * @ecc_bytes_hw:		ECC bytes used by controller hardware for this
 *				chip
 * @status:			value to be returned if NAND_CMD_STATUS command
 *				is executed
 * @last_command:		keeps track of last command on this chip. used
 *				for reading correct status
 *
 * @cfg0, cfg1, cfg0_raw..:	NANDc register configurations needed for
 *				ecc/non-ecc mode for the current nand flash
 *				device
 *
 * @quad_mode:			x4 mode for serial nand device.
 *
 * @check_qe_bit:		This flag will decide to check device
 *				configuration register for quad mode or not.
 */
struct qcom_nand_host {
	struct nand_chip chip;
	struct list_head node;

	int cs;
	int cw_size;
	int cw_data;
	bool use_ecc;
	bool bch_enabled;
	int ecc_bytes_hw;
	int spare_bytes;
	int bbm_size;
	u8 status;
	int last_command;

	u32 cfg0, cfg1;
	u32 cfg0_raw, cfg1_raw;
	u32 ecc_buf_cfg;
	u32 ecc_bch_cfg;
	u32 clrflashstatus;
	u32 clrreadstatus;
	bool quad_mode;
	bool check_qe_bit;
};

/*
 * This data type corresponds to the NAND controller properties which varies
 * among different NAND controllers.
 * @ecc_modes - ecc mode for NAND
 * @is_bam - whether NAND controller is using BAM
 * @is_qpic - whether NAND CTRL is part of qpic IP
 * @dev_cmd_reg_start - NAND_DEV_CMD_* registers starting offset
 * @is_serial_nand - QSPI nand flag, whether QPIC support serial nand or not
 * @qpic_v2 - flag to indicate QPIC IP version 2
 * @is_serial_training - flag to enable or disable serial training
 * @quad_mode - flag to enable or disable quad mode
 * @page_scope - flag to enable or disable page scope
 * @switch_layout - flag to enable or disable switching of nand page size
 * @dev_cmd_reg_start - device command register start
 */
struct qcom_nandc_props {
	u32 ecc_modes;
	bool is_bam;
	bool is_qpic;
	bool is_serial_nand;
	bool qpic_v2;
	bool is_serial_training;
	bool quad_mode;
	bool page_scope;
	bool switch_layout;
	u32 dev_cmd_reg_start;
};

/* Frees the BAM transaction memory */
static void free_bam_transaction(struct qcom_nand_controller *nandc)
{
	struct bam_transaction *bam_txn = nandc->bam_txn;

	devm_kfree(nandc->dev, bam_txn);
}

/* Allocates and Initializes the BAM transaction */
static struct bam_transaction *
alloc_bam_transaction(struct qcom_nand_controller *nandc)
{
	struct bam_transaction *bam_txn;
	size_t bam_txn_size;
	unsigned int num_cw = nandc->max_cwperpage;
	void *bam_txn_buf;

	bam_txn_size =
		sizeof(*bam_txn) + num_cw *
		((sizeof(*bam_txn->bam_ce) * QPIC_PER_CW_CMD_ELEMENTS) +
		(sizeof(*bam_txn->cmd_sgl) * QPIC_PER_CW_CMD_SGL) +
		(sizeof(*bam_txn->data_sgl) * QPIC_PER_CW_DATA_SGL));
	if (nandc->props->qpic_v2)
		bam_txn_size += sizeof(*bam_txn->sts_sgl) * QPIC_PER_CW_STS_SGL * num_cw;

	bam_txn_buf = devm_kzalloc(nandc->dev, bam_txn_size, GFP_KERNEL);
	if (!bam_txn_buf)
		return NULL;

	bam_txn = bam_txn_buf;
	bam_txn_buf += sizeof(*bam_txn);

	bam_txn->bam_ce = bam_txn_buf;
	bam_txn_buf +=
		sizeof(*bam_txn->bam_ce) * QPIC_PER_CW_CMD_ELEMENTS * num_cw;

	bam_txn->cmd_sgl = bam_txn_buf;
	bam_txn_buf +=
		sizeof(*bam_txn->cmd_sgl) * QPIC_PER_CW_CMD_SGL * num_cw;

	bam_txn->data_sgl = bam_txn_buf;

	if (nandc->props->qpic_v2) {
		bam_txn_buf +=
			sizeof(*bam_txn->data_sgl) * QPIC_PER_CW_DATA_SGL * num_cw;
		bam_txn->sts_sgl = bam_txn_buf;
	}

	init_completion(&bam_txn->txn_done);

	return bam_txn;
}

/* Clears the BAM transaction indexes */
static void clear_bam_transaction(struct qcom_nand_controller *nandc)
{
	struct bam_transaction *bam_txn = nandc->bam_txn;

	if (!nandc->props->is_bam)
		return;

	bam_txn->bam_ce_pos = 0;
	bam_txn->bam_ce_start = 0;
	bam_txn->cmd_sgl_pos = 0;
	bam_txn->cmd_sgl_start = 0;
	bam_txn->tx_sgl_pos = 0;
	bam_txn->tx_sgl_start = 0;
	bam_txn->rx_sgl_pos = 0;
	bam_txn->rx_sgl_start = 0;
	bam_txn->last_data_desc = NULL;
	bam_txn->wait_second_completion = false;

	sg_init_table(bam_txn->cmd_sgl, nandc->max_cwperpage *
		      QPIC_PER_CW_CMD_SGL);
	sg_init_table(bam_txn->data_sgl, nandc->max_cwperpage *
		      QPIC_PER_CW_DATA_SGL);
	if (nandc->props->qpic_v2) {
		bam_txn->sts_sgl_pos = 0;
		bam_txn->sts_sgl_start = 0;
		sg_init_table(bam_txn->sts_sgl, nandc->max_cwperpage *
			      QPIC_PER_CW_STS_SGL);
	}

	reinit_completion(&bam_txn->txn_done);
}

/* Callback for DMA descriptor completion */
static void qpic_bam_dma_done(void *data)
{
	struct bam_transaction *bam_txn = data;

	/*
	 * In case of data transfer with NAND, 2 callbacks will be generated.
	 * One for command channel and another one for data channel.
	 * If current transaction has data descriptors
	 * (i.e. wait_second_completion is true), then set this to false
	 * and wait for second DMA descriptor completion.
	 */
	if (bam_txn->wait_second_completion)
		bam_txn->wait_second_completion = false;
	else
		complete(&bam_txn->txn_done);
}

static inline struct qcom_nand_host *to_qcom_nand_host(struct nand_chip *chip)
{
	return container_of(chip, struct qcom_nand_host, chip);
}

static inline struct qcom_nand_controller *
get_qcom_nand_controller(struct nand_chip *chip)
{
	return container_of(chip->controller, struct qcom_nand_controller,
			    controller);
}

static inline u32 nandc_read(struct qcom_nand_controller *nandc, int offset)
{
	return ioread32(nandc->base + offset);
}

static inline void nandc_write(struct qcom_nand_controller *nandc, int offset,
			       u32 val)
{
	iowrite32(val, nandc->base + offset);
}

static inline void nandc_read_buffer_sync(struct qcom_nand_controller *nandc,
					  bool is_cpu)
{
	if (!nandc->props->is_bam)
		return;

	if (is_cpu)
		dma_sync_single_for_cpu(nandc->dev, nandc->reg_read_dma,
					MAX_REG_RD *
					sizeof(*nandc->reg_read_buf),
					DMA_FROM_DEVICE);
	else
		dma_sync_single_for_device(nandc->dev, nandc->reg_read_dma,
					   MAX_REG_RD *
					   sizeof(*nandc->reg_read_buf),
					   DMA_FROM_DEVICE);
}

static __le32 *offset_to_nandc_reg(struct nandc_regs *regs, int offset)
{
	switch (offset) {
	case NAND_FLASH_CMD:
		return &regs->cmd;
	case NAND_ADDR0:
		return &regs->addr0;
	case NAND_ADDR1:
		return &regs->addr1;
	case NAND_FLASH_CHIP_SELECT:
		return &regs->chip_sel;
	case NAND_EXEC_CMD:
		return &regs->exec;
	case NAND_FLASH_STATUS:
		return &regs->clrflashstatus;
	case NAND_DEV0_CFG0:
		return &regs->cfg0;
	case NAND_DEV0_CFG1:
		return &regs->cfg1;
	case NAND_DEV0_ECC_CFG:
		return &regs->ecc_bch_cfg;
	case NAND_AUTO_STATUS_EN:
		return &regs->auto_sts_en;
	case NAND_READ_STATUS:
		return &regs->clrreadstatus;
	case NAND_DEV_CMD1:
		return &regs->cmd1;
	case NAND_DEV_CMD1_RESTORE:
		return &regs->orig_cmd1;
	case NAND_DEV_CMD_VLD:
		return &regs->vld;
	case NAND_DEV_CMD_VLD_RESTORE:
		return &regs->orig_vld;
	case NAND_EBI2_ECC_BUF_CFG:
		return &regs->ecc_buf_cfg;
	case NAND_READ_LOCATION_0:
		return &regs->read_location0;
	case NAND_READ_LOCATION_1:
		return &regs->read_location1;
	case NAND_READ_LOCATION_2:
		return &regs->read_location2;
	case NAND_READ_LOCATION_3:
		return &regs->read_location3;
	case NAND_READ_LOCATION_LAST_CW_0:
		return &regs->read_location_last0;
	case NAND_READ_LOCATION_LAST_CW_1:
		return &regs->read_location_last1;
	case NAND_READ_LOCATION_LAST_CW_2:
		return &regs->read_location_last2;
	case NAND_READ_LOCATION_LAST_CW_3:
		return &regs->read_location_last3;
	case NAND_FLASH_SPI_CFG:
		return &regs->spi_cfg;
	case NAND_SPI_NUM_ADDR_CYCLES:
		return &regs->num_addr_cycle;
	case NAND_SPI_BUSY_CHECK_WAIT_CNT:
		return &regs->busy_wait_cnt;
	case NAND_QSPI_MSTR_CONFIG:
		return &regs->mstr_cfg;
	case NAND_FLASH_FEATURES:
		return &regs->flash_feature;
	default:
		return NULL;
	}
}

static void nandc_set_reg(struct qcom_nand_controller *nandc, int offset,
			  u32 val)
{
	struct nandc_regs *regs = nandc->regs;
	__le32 *reg;

	reg = offset_to_nandc_reg(regs, offset);

	if (reg)
		*reg = cpu_to_le32(val);
}

/* Helper to check the code word, whether it is last cw or not */
static bool qcom_nandc_is_last_cw(struct nand_ecc_ctrl *ecc, int cw)
{
	return cw == (ecc->steps - 1);
}

/* helper to configure location register values */
static void nandc_set_read_loc(struct nand_chip *chip, int cw, int reg,
		int cw_offset, int read_size, int is_last_read_loc)
{
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;

	int reg_base = NAND_READ_LOCATION_0;
	if (nandc->props->qpic_v2 && qcom_nandc_is_last_cw(ecc, cw))
		reg_base = NAND_READ_LOCATION_LAST_CW_0;
	reg_base += reg * 4;

	if (nandc->props->qpic_v2 && qcom_nandc_is_last_cw(ecc, cw))
		return nandc_set_read_loc_last(nandc, reg_base, cw_offset,
				read_size, is_last_read_loc);
	else
		return nandc_set_read_loc_first(nandc, reg_base, cw_offset,
				read_size, is_last_read_loc);
}

/* helper to configure address register values */
static void set_address(struct qcom_nand_host *host, u16 column, int page)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);

	if (chip->options & NAND_BUSWIDTH_16)
		column >>= 1;

	nandc_set_reg(nandc, NAND_ADDR0, page << 16 | column);
	nandc_set_reg(nandc, NAND_ADDR1, page >> 16 & 0xff);
}

/*
 * update_rw_regs:	set up read/write register values, these will be
 *			written to the NAND controller registers via DMA
 *
 * @num_cw:		number of steps for the read/write operation
 * @read:		read or write operation
 * @cw:			which code word
 */
static void update_rw_regs(struct qcom_nand_host *host, int num_cw, bool read, int cw)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	u32 cmd, cfg0, cfg1, ecc_bch_cfg;

	cmd = (PAGE_ACC | LAST_PAGE);

	if (nandc->props->is_serial_nand) {
		if (nandc->props->quad_mode && host->quad_mode)
			cmd |= SPI_TRANSFER_MODE_x4;
		else
			cmd |= SPI_TRANSFER_MODE_x1;
		cmd |= (SPI_WP | SPI_HOLD);
	}

	if (read) {
		if (host->use_ecc) {
			cmd |= OP_PAGE_READ_WITH_ECC;
			if (nandc->props->qpic_v2 && nandc->props->page_scope)
				cmd |= PAGE_SCOPE_READ;
		} else {
			cmd |= OP_PAGE_READ;
		}
	} else {
		cmd |= OP_PROGRAM_PAGE;
	}

	if (host->use_ecc) {
		cfg0 = (host->cfg0 & ~(7U << CW_PER_PAGE)) |
				(num_cw - 1) << CW_PER_PAGE;

		cfg1 = host->cfg1;
		ecc_bch_cfg = host->ecc_bch_cfg;
	} else {
		cfg0 = (host->cfg0_raw & ~(7U << CW_PER_PAGE)) |
				(num_cw - 1) << CW_PER_PAGE;

		cfg1 = host->cfg1_raw;
		ecc_bch_cfg = 1 << ECC_CFG_ECC_DISABLE;
	}

	nandc_set_reg(nandc, NAND_FLASH_CMD, cmd);
	nandc_set_reg(nandc, NAND_DEV0_CFG0, cfg0);
	nandc_set_reg(nandc, NAND_DEV0_CFG1, cfg1);
	nandc_set_reg(nandc, NAND_DEV0_ECC_CFG, ecc_bch_cfg);
	if (nandc->props->qpic_v2 && nandc->props->page_scope)
		nandc_set_reg(nandc, NAND_AUTO_STATUS_EN, AUTO_STS_VAL);
	nandc_set_reg(nandc, NAND_EBI2_ECC_BUF_CFG, host->ecc_buf_cfg);
	nandc_set_reg(nandc, NAND_FLASH_STATUS, host->clrflashstatus);
	nandc_set_reg(nandc, NAND_READ_STATUS, host->clrreadstatus);
	nandc_set_reg(nandc, NAND_EXEC_CMD, 1);

	if (read)
		nandc_set_read_loc(chip, cw, 0, 0, host->use_ecc ?
				host->cw_data : host->cw_size, 1);
}

/*
 * Maps the scatter gather list for DMA transfer and forms the DMA descriptor
 * for BAM. This descriptor will be added in the NAND DMA descriptor queue
 * which will be submitted to DMA engine.
 */
static int prepare_bam_async_desc(struct qcom_nand_controller *nandc,
				  struct dma_chan *chan,
				  unsigned long flags)
{
	struct desc_info *desc;
	struct scatterlist *sgl;
	unsigned int sgl_cnt;
	int ret;
	struct bam_transaction *bam_txn = nandc->bam_txn;
	enum dma_transfer_direction dir_eng;
	struct dma_async_tx_descriptor *dma_desc;

	desc = kzalloc(sizeof(*desc), GFP_KERNEL);
	if (!desc)
		return -ENOMEM;

	if (chan == nandc->cmd_chan) {
		sgl = &bam_txn->cmd_sgl[bam_txn->cmd_sgl_start];
		sgl_cnt = bam_txn->cmd_sgl_pos - bam_txn->cmd_sgl_start;
		bam_txn->cmd_sgl_start = bam_txn->cmd_sgl_pos;
		dir_eng = DMA_MEM_TO_DEV;
		desc->dir = DMA_TO_DEVICE;
	} else if (chan == nandc->tx_chan) {
		sgl = &bam_txn->data_sgl[bam_txn->tx_sgl_start];
		sgl_cnt = bam_txn->tx_sgl_pos - bam_txn->tx_sgl_start;
		bam_txn->tx_sgl_start = bam_txn->tx_sgl_pos;
		dir_eng = DMA_MEM_TO_DEV;
		desc->dir = DMA_TO_DEVICE;
	} else if (nandc->props->qpic_v2 && chan == nandc->sts_chan) {
		sgl = &bam_txn->sts_sgl[bam_txn->sts_sgl_start];
		sgl_cnt = bam_txn->sts_sgl_pos - bam_txn->sts_sgl_start;
		bam_txn->sts_sgl_start = bam_txn->sts_sgl_pos;
		dir_eng = DMA_DEV_TO_MEM;
		desc->dir = DMA_FROM_DEVICE;
	} else {
		sgl = &bam_txn->data_sgl[bam_txn->rx_sgl_start];
		sgl_cnt = bam_txn->rx_sgl_pos - bam_txn->rx_sgl_start;
		bam_txn->rx_sgl_start = bam_txn->rx_sgl_pos;
		dir_eng = DMA_DEV_TO_MEM;
		desc->dir = DMA_FROM_DEVICE;
	}

	sg_mark_end(sgl + sgl_cnt - 1);
	ret = dma_map_sg(nandc->dev, sgl, sgl_cnt, desc->dir);
	if (ret == 0) {
		dev_err(nandc->dev, "failure in mapping desc\n");
		kfree(desc);
		return -ENOMEM;
	}

	desc->sgl_cnt = sgl_cnt;
	desc->bam_sgl = sgl;

	dma_desc = dmaengine_prep_slave_sg(chan, sgl, sgl_cnt, dir_eng,
					   flags);

	if (!dma_desc) {
		dev_err(nandc->dev, "failure in prep desc\n");
		dma_unmap_sg(nandc->dev, sgl, sgl_cnt, desc->dir);
		kfree(desc);
		return -EINVAL;
	}

	desc->dma_desc = dma_desc;

	/* update last data/command descriptor */
	if (chan == nandc->cmd_chan)
		bam_txn->last_cmd_desc = dma_desc;
	else
		bam_txn->last_data_desc = dma_desc;

	list_add_tail(&desc->node, &nandc->desc_list);

	return 0;
}

/*
 * Prepares the command descriptor for BAM DMA which will be used for NAND
 * register reads and writes. The command descriptor requires the command
 * to be formed in command element type so this function uses the command
 * element from bam transaction ce array and fills the same with required
 * data. A single SGL can contain multiple command elements so
 * NAND_BAM_NEXT_SGL will be used for starting the separate SGL
 * after the current command element.
 */
static int prep_bam_dma_desc_cmd(struct qcom_nand_controller *nandc, bool read,
				 int reg_off, const void *vaddr,
				 int size, unsigned int flags)
{
	int bam_ce_size;
	int i, ret;
	struct bam_cmd_element *bam_ce_buffer;
	struct bam_transaction *bam_txn = nandc->bam_txn;

	bam_ce_buffer = &bam_txn->bam_ce[bam_txn->bam_ce_pos];

	/* fill the command desc */
	for (i = 0; i < size; i++) {
		if (read)
			bam_prep_ce(&bam_ce_buffer[i],
				    nandc_reg_phys(nandc, reg_off + 4 * i),
				    BAM_READ_COMMAND,
				    reg_buf_dma_addr(nandc,
						     (__le32 *)vaddr + i));
		else
			bam_prep_ce_le32(&bam_ce_buffer[i],
					 nandc_reg_phys(nandc, reg_off + 4 * i),
					 BAM_WRITE_COMMAND,
					 *((__le32 *)vaddr + i));
	}

	bam_txn->bam_ce_pos += size;

	/* use the separate sgl after this command */
	if (flags & NAND_BAM_NEXT_SGL) {
		bam_ce_buffer = &bam_txn->bam_ce[bam_txn->bam_ce_start];
		bam_ce_size = (bam_txn->bam_ce_pos -
				bam_txn->bam_ce_start) *
				sizeof(struct bam_cmd_element);
		sg_set_buf(&bam_txn->cmd_sgl[bam_txn->cmd_sgl_pos],
			   bam_ce_buffer, bam_ce_size);
		bam_txn->cmd_sgl_pos++;
		bam_txn->bam_ce_start = bam_txn->bam_ce_pos;

		if (flags & NAND_BAM_NWD) {
			ret = prepare_bam_async_desc(nandc, nandc->cmd_chan,
						     DMA_PREP_FENCE |
						     DMA_PREP_CMD);
			if (ret)
				return ret;
		}
	}

	return 0;
}

/*
 * Prepares the data descriptor for BAM DMA which will be used for NAND
 * data reads and writes.
 */
static int prep_bam_dma_desc_data(struct qcom_nand_controller *nandc, bool read,
				  const void *vaddr,
				  int size, unsigned int flags)
{
	int ret;
	struct bam_transaction *bam_txn = nandc->bam_txn;

	if (read) {
		sg_set_buf(&bam_txn->data_sgl[bam_txn->rx_sgl_pos],
			   vaddr, size);
		bam_txn->rx_sgl_pos++;
	} else {
		sg_set_buf(&bam_txn->data_sgl[bam_txn->tx_sgl_pos],
			   vaddr, size);
		bam_txn->tx_sgl_pos++;

		/*
		 * BAM will only set EOT for DMA_PREP_INTERRUPT so if this flag
		 * is not set, form the DMA descriptor
		 */
		if (!(flags & NAND_BAM_NO_EOT)) {
			ret = prepare_bam_async_desc(nandc, nandc->tx_chan,
						     DMA_PREP_INTERRUPT);
			if (ret)
				return ret;
		}
	}

	return 0;
}

static int prep_adm_dma_desc(struct qcom_nand_controller *nandc, bool read,
			     int reg_off, const void *vaddr, int size,
			     bool flow_control)
{
	struct desc_info *desc;
	struct dma_async_tx_descriptor *dma_desc;
	struct scatterlist *sgl;
	struct dma_slave_config slave_conf;
	enum dma_transfer_direction dir_eng;
	int ret;

	desc = kzalloc(sizeof(*desc), GFP_KERNEL);
	if (!desc)
		return -ENOMEM;

	sgl = &desc->adm_sgl;

	sg_init_one(sgl, vaddr, size);

	if (read) {
		dir_eng = DMA_DEV_TO_MEM;
		desc->dir = DMA_FROM_DEVICE;
	} else {
		dir_eng = DMA_MEM_TO_DEV;
		desc->dir = DMA_TO_DEVICE;
	}

	ret = dma_map_sg(nandc->dev, sgl, 1, desc->dir);
	if (ret == 0) {
		ret = -ENOMEM;
		goto err;
	}

	memset(&slave_conf, 0x00, sizeof(slave_conf));

	slave_conf.device_fc = flow_control;
	if (read) {
		slave_conf.src_maxburst = 16;
		slave_conf.src_addr = nandc->base_dma + reg_off;
		slave_conf.slave_id = nandc->data_crci;
	} else {
		slave_conf.dst_maxburst = 16;
		slave_conf.dst_addr = nandc->base_dma + reg_off;
		slave_conf.slave_id = nandc->cmd_crci;
	}

	ret = dmaengine_slave_config(nandc->chan, &slave_conf);
	if (ret) {
		dev_err(nandc->dev, "failed to configure dma channel\n");
		goto err;
	}

	dma_desc = dmaengine_prep_slave_sg(nandc->chan, sgl, 1, dir_eng, 0);
	if (!dma_desc) {
		dev_err(nandc->dev, "failed to prepare desc\n");
		ret = -EINVAL;
		goto err;
	}

	desc->dma_desc = dma_desc;

	list_add_tail(&desc->node, &nandc->desc_list);

	return 0;
err:
	kfree(desc);

	return ret;
}

/*
 * read_reg_dma:	prepares a descriptor to read a given number of
 *			contiguous registers to the reg_read_buf pointer
 *
 * @first:		offset of the first register in the contiguous block
 * @num_regs:		number of registers to read
 * @flags:		flags to control DMA descriptor preparation
 */
static int read_reg_dma(struct qcom_nand_controller *nandc, int first,
			int num_regs, unsigned int flags)
{
	bool flow_control = false;
	void *vaddr;

	vaddr = nandc->reg_read_buf + nandc->reg_read_pos;
	nandc->reg_read_pos += num_regs;

	if (first == NAND_DEV_CMD_VLD || first == NAND_DEV_CMD1)
		first = dev_cmd_reg_addr(nandc, first);

	if (nandc->props->is_bam)
		return prep_bam_dma_desc_cmd(nandc, true, first, vaddr,
					     num_regs, flags);

	if (first == NAND_READ_ID || first == NAND_FLASH_STATUS)
		flow_control = true;

	return prep_adm_dma_desc(nandc, true, first, vaddr,
				 num_regs * sizeof(u32), flow_control);
}

/*
 * write_reg_dma:	prepares a descriptor to write a given number of
 *			contiguous registers
 *
 * @first:		offset of the first register in the contiguous block
 * @num_regs:		number of registers to write
 * @flags:		flags to control DMA descriptor preparation
 */
static int write_reg_dma(struct qcom_nand_controller *nandc, int first,
			 int num_regs, unsigned int flags)
{
	bool flow_control = false;
	struct nandc_regs *regs = nandc->regs;
	void *vaddr;

	vaddr = offset_to_nandc_reg(regs, first);

	if (first == NAND_ERASED_CW_DETECT_CFG) {
		if (flags & NAND_ERASED_CW_SET)
			vaddr = &regs->erased_cw_detect_cfg_set;
		else
			vaddr = &regs->erased_cw_detect_cfg_clr;
	}

	if (!vaddr)
		return -EINVAL;

	if (first == NAND_EXEC_CMD)
		flags |= NAND_BAM_NWD;

	if (first == NAND_FLASH_SPI_CFG || first == NAND_SPI_NUM_ADDR_CYCLES
		|| first == NAND_SPI_BUSY_CHECK_WAIT_CNT
		|| first == NAND_QSPI_MSTR_CONFIG)
		first = dev_cmd_reg_addr(nandc, first);

	if (first == NAND_DEV_CMD1_RESTORE || first == NAND_DEV_CMD1)
		first = dev_cmd_reg_addr(nandc, NAND_DEV_CMD1);

	if (first == NAND_DEV_CMD_VLD_RESTORE || first == NAND_DEV_CMD_VLD)
		first = dev_cmd_reg_addr(nandc, NAND_DEV_CMD_VLD);

	if (nandc->props->is_bam)
		return prep_bam_dma_desc_cmd(nandc, false, first, vaddr,
					     num_regs, flags);

	if (first == NAND_FLASH_CMD)
		flow_control = true;

	return prep_adm_dma_desc(nandc, false, first, vaddr,
				 num_regs * sizeof(u32), flow_control);
}

/*
 * read_status_data_dma: prepares a DMA descriptor to transfer status from the
 * 			 controller's status registers to buffer 'vaddr'
 *
 * @reg_off:            offset within the controller's data buffer
 * @vaddr:              virtual address of the buffer we want to write to
 * @size:               DMA transaction size in bytes
 * @flags:              flags to control DMA descriptor preparation
 */
static int read_status_data_dma(struct qcom_nand_controller *nandc, int reg_off,
		const u8 *vaddr, int size, unsigned int flags)
{
	struct bam_transaction *bam_txn = nandc->bam_txn;

	sg_set_buf(&bam_txn->sts_sgl[bam_txn->sts_sgl_pos],
			vaddr, size);
	bam_txn->sts_sgl_pos++;

	return 0;
}

/*
 * read_data_dma:	prepares a DMA descriptor to transfer data from the
 *			controller's internal buffer to the buffer 'vaddr'
 *
 * @reg_off:		offset within the controller's data buffer
 * @vaddr:		virtual address of the buffer we want to write to
 * @size:		DMA transaction size in bytes
 * @flags:		flags to control DMA descriptor preparation
 */
static int read_data_dma(struct qcom_nand_controller *nandc, int reg_off,
			 const u8 *vaddr, int size, unsigned int flags)
{
	if (nandc->props->is_bam)
		return prep_bam_dma_desc_data(nandc, true, vaddr, size, flags);

	return prep_adm_dma_desc(nandc, true, reg_off, vaddr, size, false);
}

/*
 * write_data_dma:	prepares a DMA descriptor to transfer data from
 *			'vaddr' to the controller's internal buffer
 *
 * @reg_off:		offset within the controller's data buffer
 * @vaddr:		virtual address of the buffer we want to read from
 * @size:		DMA transaction size in bytes
 * @flags:		flags to control DMA descriptor preparation
 */
static int write_data_dma(struct qcom_nand_controller *nandc, int reg_off,
			  const u8 *vaddr, int size, unsigned int flags)
{
	if (nandc->props->is_bam)
		return prep_bam_dma_desc_data(nandc, false, vaddr, size, flags);

	return prep_adm_dma_desc(nandc, false, reg_off, vaddr, size, false);
}

/*
 * Helper to prepare DMA descriptors for configuring registers
 * before reading a NAND page.
 */
static void config_nand_page_read(struct qcom_nand_controller *nandc)
{
	write_reg_dma(nandc, NAND_ADDR0, 2, 0);
	write_reg_dma(nandc, NAND_DEV0_CFG0, 3, 0);
	write_reg_dma(nandc, NAND_EBI2_ECC_BUF_CFG, 1, 0);
	write_reg_dma(nandc, NAND_ERASED_CW_DETECT_CFG, 1, 0);
	write_reg_dma(nandc, NAND_ERASED_CW_DETECT_CFG, 1,
		      NAND_ERASED_CW_SET | NAND_BAM_NEXT_SGL);
}

/* helper to check which location register should be use for this
 * code word. NAND_READ_LOCATION or NAND_READ_LOCATION_LAST_CW
 */
static bool config_loc_last_reg(struct nand_chip *chip, int cw)
{
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;

	if (nandc->props->qpic_v2 && qcom_nandc_is_last_cw(ecc, cw))
		return true;

	return false;
}

/*
 * Helper to prepare DMA descriptors for configuring registers
 * before reading each codeword in NAND page.
 */
static void
config_nand_cw_read(struct nand_chip *chip, bool use_ecc, int cw)
{
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	int reg = NAND_READ_LOCATION_0;
	struct nand_ecc_ctrl *ecc = &chip->ecc;

	if (config_loc_last_reg(chip, cw))
		reg = NAND_READ_LOCATION_LAST_CW_0;

	if (nandc->props->is_bam)
		write_reg_dma(nandc, reg, 4, NAND_BAM_NEXT_SGL);

	write_reg_dma(nandc, NAND_FLASH_CMD, 1, NAND_BAM_NEXT_SGL);

	if (use_ecc) {
		if (nandc->props->qpic_v2 && nandc->props->page_scope) {
			write_reg_dma(nandc, NAND_AUTO_STATUS_EN, 1, NAND_BAM_NEXT_SGL);
			if (qcom_nandc_is_last_cw(ecc, cw))
				write_reg_dma(nandc, NAND_EXEC_CMD, 1, NAND_BAM_NEXT_SGL);
		} else {
			write_reg_dma(nandc, NAND_EXEC_CMD, 1, NAND_BAM_NEXT_SGL);
			read_reg_dma(nandc, NAND_FLASH_STATUS, 2, 0);
			read_reg_dma(nandc, NAND_ERASED_CW_DETECT_STATUS, 1,
			       NAND_BAM_NEXT_SGL);
		}
	} else {
		write_reg_dma(nandc, NAND_EXEC_CMD, 1, NAND_BAM_NEXT_SGL);
		read_reg_dma(nandc, NAND_FLASH_STATUS, 1, NAND_BAM_NEXT_SGL);
	}
}

/*
 * Helper to prepare dma descriptors to configure registers needed for reading a
 * single codeword in page
 */
static void
config_nand_single_cw_page_read(struct nand_chip *chip,
				bool use_ecc, int cw)
{
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);

	config_nand_page_read(nandc);
	config_nand_cw_read(chip, use_ecc, cw);
}

/*
 * Helper to prepare DMA descriptors used to configure registers needed for
 * before writing a NAND page.
 */
static void config_nand_page_write(struct nand_chip *chip)
{
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);

	write_reg_dma(nandc, NAND_ADDR0, 2, 0);
	write_reg_dma(nandc, NAND_DEV0_CFG0, 3, 0);
	write_reg_dma(nandc, NAND_EBI2_ECC_BUF_CFG, 1,
		      NAND_BAM_NEXT_SGL);
}

/*
 * Helper to prepare DMA descriptors for configuring registers
 * before writing each codeword in NAND page.
 */
static void config_nand_cw_write(struct nand_chip *chip)
{
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);

	write_reg_dma(nandc, NAND_FLASH_CMD, 1, NAND_BAM_NEXT_SGL);
	write_reg_dma(nandc, NAND_EXEC_CMD, 1, NAND_BAM_NEXT_SGL);

	read_reg_dma(nandc, NAND_FLASH_STATUS, 1, NAND_BAM_NEXT_SGL);

	write_reg_dma(nandc, NAND_FLASH_STATUS, 1, 0);
	write_reg_dma(nandc, NAND_READ_STATUS, 1, NAND_BAM_NEXT_SGL);
}

/*
 * the following functions are used within chip->legacy.cmdfunc() to
 * perform different NAND_CMD_* commands
 */

/* sets up descriptors for NAND_CMD_PARAM */
static int nandc_param(struct qcom_nand_host *host)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);

	/*
	 * NAND_CMD_PARAM is called before we know much about the FLASH chip
	 * in use. we configure the controller to perform a raw read of 512
	 * bytes to read onfi params
	 */
	nandc_set_reg(nandc, NAND_FLASH_CMD, OP_PAGE_READ | PAGE_ACC | LAST_PAGE);
	nandc_set_reg(nandc, NAND_ADDR0, 0);
	nandc_set_reg(nandc, NAND_ADDR1, 0);
	nandc_set_reg(nandc, NAND_DEV0_CFG0, 0 << CW_PER_PAGE
					| 512 << UD_SIZE_BYTES
					| 1 << NUM_ADDR_CYCLES
					| 0 << SPARE_SIZE_BYTES);
	nandc_set_reg(nandc, NAND_DEV0_CFG1, 7 << NAND_RECOVERY_CYCLES
					| 0 << CS_ACTIVE_BSY
					| 17 << BAD_BLOCK_BYTE_NUM
					| 1 << BAD_BLOCK_IN_SPARE_AREA
					| 2 << WR_RD_BSY_GAP
					| 0 << WIDE_FLASH
					| 1 << DEV0_CFG1_ECC_DISABLE);
	nandc_set_reg(nandc, NAND_EBI2_ECC_BUF_CFG, 1 << ECC_CFG_ECC_DISABLE);

	/* configure CMD1 and VLD for ONFI param probing */
	nandc_set_reg(nandc, NAND_DEV_CMD_VLD,
		      (nandc->vld & ~READ_START_VLD));
	nandc_set_reg(nandc, NAND_DEV_CMD1,
		      (nandc->cmd1 & ~(0xFF << READ_ADDR))
		      | NAND_CMD_PARAM << READ_ADDR);

	nandc_set_reg(nandc, NAND_EXEC_CMD, 1);

	nandc_set_reg(nandc, NAND_DEV_CMD1_RESTORE, nandc->cmd1);
	nandc_set_reg(nandc, NAND_DEV_CMD_VLD_RESTORE, nandc->vld);
	nandc_set_read_loc(chip, 0, 0, 0, 512, 1);

	write_reg_dma(nandc, NAND_DEV_CMD_VLD, 1, 0);
	write_reg_dma(nandc, NAND_DEV_CMD1, 1, NAND_BAM_NEXT_SGL);

	nandc->buf_count = 512;
	memset(nandc->data_buffer, 0xff, nandc->buf_count);

	config_nand_single_cw_page_read(chip, false, 0);

	read_data_dma(nandc, FLASH_BUF_ACC, nandc->data_buffer,
		      nandc->buf_count, 0);

	/* restore CMD1 and VLD regs */
	write_reg_dma(nandc, NAND_DEV_CMD1_RESTORE, 1, 0);
	write_reg_dma(nandc, NAND_DEV_CMD_VLD_RESTORE, 1, NAND_BAM_NEXT_SGL);

	return 0;
}

/* sets up descriptors for NAND_CMD_ERASE1 */
static int erase_block(struct qcom_nand_host *host, int page_addr)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	u32 ers_cmd = OP_BLOCK_ERASE | PAGE_ACC | LAST_PAGE;
	u32 addr1 = 0x0;

	if (nandc->props->is_serial_nand) {
		ers_cmd |= (SPI_WP | SPI_HOLD | SPI_TRANSFER_MODE_x1);
		addr1 = (page_addr >> 16) & 0xffff;
		page_addr <<= 16;
	}
	nandc_set_reg(nandc, NAND_FLASH_CMD, ers_cmd);
	nandc_set_reg(nandc, NAND_ADDR0, page_addr);
	nandc_set_reg(nandc, NAND_ADDR1, addr1);
	nandc_set_reg(nandc, NAND_DEV0_CFG0,
		      host->cfg0_raw & ~(7 << CW_PER_PAGE));
	nandc_set_reg(nandc, NAND_DEV0_CFG1, host->cfg1_raw);
	nandc_set_reg(nandc, NAND_EXEC_CMD, 1);
	nandc_set_reg(nandc, NAND_FLASH_STATUS, host->clrflashstatus);
	nandc_set_reg(nandc, NAND_READ_STATUS, host->clrreadstatus);

	write_reg_dma(nandc, NAND_FLASH_CMD, 3, NAND_BAM_NEXT_SGL);
	write_reg_dma(nandc, NAND_DEV0_CFG0, 2, NAND_BAM_NEXT_SGL);
	write_reg_dma(nandc, NAND_EXEC_CMD, 1, NAND_BAM_NEXT_SGL);

	read_reg_dma(nandc, NAND_FLASH_STATUS, 1, NAND_BAM_NEXT_SGL);

	write_reg_dma(nandc, NAND_FLASH_STATUS, 1, 0);
	write_reg_dma(nandc, NAND_READ_STATUS, 1, NAND_BAM_NEXT_SGL);

	return 0;
}

/* sets up descriptors for NAND_CMD_READID */
static int read_id(struct qcom_nand_host *host, int column)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	u32 cmd = OP_FETCH_ID;

	if (column == -1)
		return 0;

	if (nandc->props->is_serial_nand) {
		cmd |= (SPI_WP | SPI_HOLD | SPI_TRANSFER_MODE_x1);
		/* For spi nand read 2-bytes id only
		 * else if nandc->buf_count == 4; then the id value
		 * will repeat and the SLC device will be detect as MLC.
		 * by nand base layer
		 * so overwrite the nandc->buf_count == 2;
		 */
		nandc->buf_count = 2;
	}

	nandc_set_reg(nandc, NAND_FLASH_CMD, cmd);
	nandc_set_reg(nandc, NAND_ADDR0, column);
	nandc_set_reg(nandc, NAND_ADDR1, 0);
	nandc_set_reg(nandc, NAND_FLASH_CHIP_SELECT,
		      nandc->props->is_bam ? 0 : DM_EN);
	nandc_set_reg(nandc, NAND_EXEC_CMD, 1);

	write_reg_dma(nandc, NAND_FLASH_CMD, 4, NAND_BAM_NEXT_SGL);
	write_reg_dma(nandc, NAND_EXEC_CMD, 1, NAND_BAM_NEXT_SGL);

	read_reg_dma(nandc, NAND_READ_ID, 1, NAND_BAM_NEXT_SGL);

	if (nandc->props->is_serial_nand &&
		((le32_to_cpu(nandc->reg_read_buf[0]) & 0xFF) ==
		NAND_MID_WINBOND)) {
		nandc->buf_count = 4;
	}

	return 0;
}

/* sets up descriptors for NAND_CMD_RESET */
static int reset(struct qcom_nand_host *host)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	int cmd_rst;

	cmd_rst = OP_RESET_DEVICE;
	if (nandc->props->is_serial_nand)
		cmd_rst |= (SPI_WP | SPI_HOLD | SPI_TRANSFER_MODE_x1);

	nandc_set_reg(nandc, NAND_FLASH_CMD, cmd_rst);
	nandc_set_reg(nandc, NAND_EXEC_CMD, 1);

	write_reg_dma(nandc, NAND_FLASH_CMD, 1, NAND_BAM_NEXT_SGL);
	write_reg_dma(nandc, NAND_EXEC_CMD, 1, NAND_BAM_NEXT_SGL);

	read_reg_dma(nandc, NAND_FLASH_STATUS, 1, NAND_BAM_NEXT_SGL);

	return 0;
}

/* helpers to submit/free our list of dma descriptors */
static int submit_descs(struct qcom_nand_controller *nandc)
{
	struct desc_info *desc;
	dma_cookie_t cookie = 0;
	struct bam_transaction *bam_txn = nandc->bam_txn;
	int r;

	if (nandc->props->is_bam) {
		if (bam_txn->rx_sgl_pos > bam_txn->rx_sgl_start) {
			r = prepare_bam_async_desc(nandc, nandc->rx_chan, 0);
			if (r)
				return r;
		}

		if (bam_txn->tx_sgl_pos > bam_txn->tx_sgl_start) {
			r = prepare_bam_async_desc(nandc, nandc->tx_chan,
						   DMA_PREP_INTERRUPT);
			if (r)
				return r;
		}

		if (bam_txn->cmd_sgl_pos > bam_txn->cmd_sgl_start) {
			r = prepare_bam_async_desc(nandc, nandc->cmd_chan,
						   DMA_PREP_CMD);
			if (r)
				return r;
		}

		if (nandc->props->qpic_v2) {
			if (bam_txn->sts_sgl_pos > bam_txn->sts_sgl_start) {
				r = prepare_bam_async_desc(nandc, nandc->sts_chan, 0);
				if (r)
					return r;
			}
		}
	}

	list_for_each_entry(desc, &nandc->desc_list, node)
		cookie = dmaengine_submit(desc->dma_desc);

	if (nandc->props->is_bam) {
		bam_txn->last_cmd_desc->callback = qpic_bam_dma_done;
		bam_txn->last_cmd_desc->callback_param = bam_txn;
		if (bam_txn->last_data_desc) {
			bam_txn->last_data_desc->callback = qpic_bam_dma_done;
			bam_txn->last_data_desc->callback_param = bam_txn;
			bam_txn->wait_second_completion = true;
		}

		dma_async_issue_pending(nandc->tx_chan);
		dma_async_issue_pending(nandc->rx_chan);
		dma_async_issue_pending(nandc->cmd_chan);
		if (nandc->props->qpic_v2)
			dma_async_issue_pending(nandc->sts_chan);

		if (!wait_for_completion_timeout(&bam_txn->txn_done,
						 QPIC_NAND_COMPLETION_TIMEOUT))
			return -ETIMEDOUT;
	} else {
		if (dma_sync_wait(nandc->chan, cookie) != DMA_COMPLETE)
			return -ETIMEDOUT;
	}

	return 0;
}

static void free_descs(struct qcom_nand_controller *nandc)
{
	struct desc_info *desc, *n;

	list_for_each_entry_safe(desc, n, &nandc->desc_list, node) {
		list_del(&desc->node);

		if (nandc->props->is_bam)
			dma_unmap_sg(nandc->dev, desc->bam_sgl,
				     desc->sgl_cnt, desc->dir);
		else
			dma_unmap_sg(nandc->dev, &desc->adm_sgl, 1,
				     desc->dir);

		kfree(desc);
	}
}

/* reset the register read buffer for next NAND operation */
static void clear_read_regs(struct qcom_nand_controller *nandc)
{
	nandc->reg_read_pos = 0;
	nandc_read_buffer_sync(nandc, false);
}

static void pre_command(struct qcom_nand_host *host, int command)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);

	nandc->buf_count = 0;
	nandc->buf_start = 0;
	host->use_ecc = false;
	host->last_command = command;

	clear_read_regs(nandc);

	if (command == NAND_CMD_RESET || command == NAND_CMD_READID ||
	    command == NAND_CMD_PARAM || command == NAND_CMD_ERASE1 ||
	    command == NAND_CMD_READID_SERIAL)
		clear_bam_transaction(nandc);
}

/*
 * this is called after NAND_CMD_PAGEPROG and NAND_CMD_ERASE1 to set our
 * privately maintained status byte, this status byte can be read after
 * NAND_CMD_STATUS is called
 */
static void parse_erase_write_errors(struct qcom_nand_host *host, int command)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	int num_cw;
	int i;

	num_cw = command == NAND_CMD_PAGEPROG ? ecc->steps : 1;
	nandc_read_buffer_sync(nandc, true);

	for (i = 0; i < num_cw; i++) {
		u32 flash_status = le32_to_cpu(nandc->reg_read_buf[i]);

		if (flash_status & FS_MPU_ERR)
			host->status &= ~NAND_STATUS_WP;

		if (flash_status & FS_OP_ERR || (i == (num_cw - 1) &&
						 (flash_status &
						  FS_DEVICE_STS_ERR)))
			host->status |= NAND_STATUS_FAIL;
	}
}

static void post_command(struct qcom_nand_host *host, int command)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);

	switch (command) {
	case NAND_CMD_READID:
		nandc_read_buffer_sync(nandc, true);
		memcpy(nandc->data_buffer, nandc->reg_read_buf,
		       nandc->buf_count);
		break;
	case NAND_CMD_PAGEPROG:
	case NAND_CMD_ERASE1:
		parse_erase_write_errors(host, command);
		break;
	default:
		break;
	}
}

/*
 * Implements chip->legacy.cmdfunc. It's  only used for a limited set of
 * commands. The rest of the commands wouldn't be called by upper layers.
 * For example, NAND_CMD_READOOB would never be called because we have our own
 * versions of read_oob ops for nand_ecc_ctrl.
 */
static void qcom_nandc_command(struct nand_chip *chip, unsigned int command,
			       int column, int page_addr)
{
	struct qcom_nand_host *host = to_qcom_nand_host(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	bool wait = false;
	int ret = 0;

	pre_command(host, command);

	switch (command) {
	case NAND_CMD_RESET:
		ret = reset(host);
		wait = true;
		break;

	case NAND_CMD_READID:
		nandc->buf_count = 4;
		ret = read_id(host, column);
		wait = true;
		break;

	case NAND_CMD_PARAM:
		ret = nandc_param(host);
		wait = true;
		break;

	case NAND_CMD_ERASE1:
		ret = erase_block(host, page_addr);
		wait = true;
		break;

	case NAND_CMD_READ0:
		/* we read the entire page for now */
		WARN_ON(column != 0);

		host->use_ecc = true;
		set_address(host, 0, page_addr);
		update_rw_regs(host, ecc->steps, true, 0);
		break;

	case NAND_CMD_SEQIN:
		WARN_ON(column != 0);
		set_address(host, 0, page_addr);
		break;

	case NAND_CMD_PAGEPROG:
	case NAND_CMD_STATUS:
	case NAND_CMD_NONE:
	default:
		break;
	}

	if (ret) {
		dev_err(nandc->dev, "failure executing command %d\n",
			command);
		free_descs(nandc);
		return;
	}

	if (wait) {
		ret = submit_descs(nandc);
		if (ret)
			dev_err(nandc->dev,
				"failure submitting descs for command %d\n",
				command);
	}

	free_descs(nandc);

	post_command(host, command);
}

/*
 * when using BCH ECC, the HW flags an error in NAND_FLASH_STATUS if it read
 * an erased CW, and reports an erased CW in NAND_ERASED_CW_DETECT_STATUS.
 *
 * when using RS ECC, the HW reports the same erros when reading an erased CW,
 * but it notifies that it is an erased CW by placing special characters at
 * certain offsets in the buffer.
 *
 * verify if the page is erased or not, and fix up the page for RS ECC by
 * replacing the special characters with 0xff.
 */
static bool erased_chunk_check_and_fixup(u8 *data_buf, int data_len)
{
	u8 empty1, empty2;

	/*
	 * an erased page flags an error in NAND_FLASH_STATUS, check if the page
	 * is erased by looking for 0x54s at offsets 3 and 175 from the
	 * beginning of each codeword
	 */

	empty1 = data_buf[3];
	empty2 = data_buf[175];

	/*
	 * if the erased codework markers, if they exist override them with
	 * 0xffs
	 */
	if ((empty1 == 0x54 && empty2 == 0xff) ||
	    (empty1 == 0xff && empty2 == 0x54)) {
		data_buf[3] = 0xff;
		data_buf[175] = 0xff;
	}

	/*
	 * check if the entire chunk contains 0xffs or not. if it doesn't, then
	 * restore the original values at the special offsets
	 */
	if (memchr_inv(data_buf, 0xff, data_len)) {
		data_buf[3] = empty1;
		data_buf[175] = empty2;

		return false;
	}

	return true;
}

struct read_stats {
	__le32 flash;
	__le32 buffer;
	__le32 erased_cw;
};

/* reads back FLASH_STATUS register set by the controller */
static int check_flash_errors(struct qcom_nand_host *host, int cw_cnt)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	int i;

	nandc_read_buffer_sync(nandc, true);

	for (i = 0; i < cw_cnt; i++) {
		u32 flash = le32_to_cpu(nandc->reg_read_buf[i]);

		if (flash & (FS_OP_ERR | FS_MPU_ERR))
			return -EIO;
	}

	return 0;
}

/* performs raw read for one codeword */
static int
qcom_nandc_read_cw_raw(struct mtd_info *mtd, struct nand_chip *chip,
		       u8 *data_buf, u8 *oob_buf, int page, int cw)
{
	struct qcom_nand_host *host = to_qcom_nand_host(chip);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	int data_size1, data_size2, oob_size1, oob_size2;
	int ret, reg_off = FLASH_BUF_ACC, read_loc = 0;
	int new_cw = cw;

	nand_read_page_op(chip, page, 0, NULL, 0);
	host->use_ecc = false;

	if (nandc->props->qpic_v2)
		new_cw = ecc->steps - 1;

	clear_bam_transaction(nandc);
	set_address(host, host->cw_size * cw, page);
	update_rw_regs(host, 1, true, new_cw);
	config_nand_page_read(nandc);

	data_size1 = mtd->writesize - host->cw_size * (ecc->steps - 1);
	oob_size1 = host->bbm_size;

	if (qcom_nandc_is_last_cw(ecc, cw)) {
		data_size2 = ecc->size - data_size1 -
			     ((ecc->steps - 1) * 4);
		oob_size2 = (ecc->steps * 4) + host->ecc_bytes_hw +
			    host->spare_bytes;
	} else {
		data_size2 = host->cw_data - data_size1;
		oob_size2 = host->ecc_bytes_hw + host->spare_bytes;
	}

	if (nandc->props->is_bam) {
		nandc_set_read_loc(chip, cw, 0, read_loc, data_size1, 0);
		read_loc += data_size1;

		nandc_set_read_loc(chip, cw, 1, read_loc, oob_size1, 0);
		read_loc += oob_size1;

		nandc_set_read_loc(chip, cw, 2, read_loc, data_size2, 0);
		read_loc += data_size2;

		nandc_set_read_loc(chip, cw, 3, read_loc, oob_size2, 1);
	}

	config_nand_cw_read(chip, false, new_cw);

	read_data_dma(nandc, reg_off, data_buf, data_size1, 0);
	reg_off += data_size1;

	read_data_dma(nandc, reg_off, oob_buf, oob_size1, 0);
	reg_off += oob_size1;

	read_data_dma(nandc, reg_off, data_buf + data_size1, data_size2, 0);
	reg_off += data_size2;

	read_data_dma(nandc, reg_off, oob_buf + oob_size1, oob_size2, 0);

	ret = submit_descs(nandc);
	free_descs(nandc);
	if (ret) {
		dev_err(nandc->dev, "failure to read raw cw %d\n", cw);
		return ret;
	}

	return check_flash_errors(host, 1);
}

/*
 * Bitflips can happen in erased codewords also so this function counts the
 * number of 0 in each CW for which ECC engine returns the uncorrectable
 * error. The page will be assumed as erased if this count is less than or
 * equal to the ecc->strength for each CW.
 *
 * 1. Both DATA and OOB need to be checked for number of 0. The
 *    top-level API can be called with only data buf or OOB buf so use
 *    chip->data_buf if data buf is null and chip->oob_poi if oob buf
 *    is null for copying the raw bytes.
 * 2. Perform raw read for all the CW which has uncorrectable errors.
 * 3. For each CW, check the number of 0 in cw_data and usable OOB bytes.
 *    The BBM and spare bytes bit flip won’t affect the ECC so don’t check
 *    the number of bitflips in this area.
 */
static int
check_for_erased_page(struct qcom_nand_host *host, u8 *data_buf,
		      u8 *oob_buf, unsigned long uncorrectable_cws,
		      int page, unsigned int max_bitflips)
{
	struct nand_chip *chip = &host->chip;
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	u8 *cw_data_buf, *cw_oob_buf;
	int cw, data_size, oob_size, ret = 0;

	if (!data_buf)
		data_buf = nand_get_data_buf(chip);

	if (!oob_buf) {
		nand_get_data_buf(chip);
		oob_buf = chip->oob_poi;
	}

	for_each_set_bit(cw, &uncorrectable_cws, ecc->steps) {
		if (qcom_nandc_is_last_cw(ecc, cw)) {
			data_size = ecc->size - ((ecc->steps - 1) * 4);
			oob_size = (ecc->steps * 4) + host->ecc_bytes_hw;
		} else {
			data_size = host->cw_data;
			oob_size = host->ecc_bytes_hw;
		}

		/* determine starting buffer address for current CW */
		cw_data_buf = data_buf + (cw * host->cw_data);
		cw_oob_buf = oob_buf + (cw * ecc->bytes);

		ret = qcom_nandc_read_cw_raw(mtd, chip, cw_data_buf,
					     cw_oob_buf, page, cw);
		if (ret)
			return ret;

		/*
		 * make sure it isn't an erased page reported
		 * as not-erased by HW because of a few bitflips
		 */
		ret = nand_check_erased_ecc_chunk(cw_data_buf, data_size,
						  cw_oob_buf + host->bbm_size,
						  oob_size, NULL,
						  0, ecc->strength);
		if (ret < 0) {
			mtd->ecc_stats.failed++;
		} else {
			mtd->ecc_stats.corrected += ret;
			max_bitflips = max_t(unsigned int, max_bitflips, ret);
		}
	}

	return max_bitflips;
}

/*
 * reads back status registers set by the controller to notify page read
 * errors. this is equivalent to what 'ecc->correct()' would do.
 */
static int parse_read_errors(struct qcom_nand_host *host, u8 *data_buf,
			     u8 *oob_buf, int page)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	unsigned int max_bitflips = 0, uncorrectable_cws = 0;
	struct read_stats *buf;
	bool flash_op_err = false, erased;
	int i;
	u8 *data_buf_start = data_buf, *oob_buf_start = oob_buf;

	buf = (struct read_stats *)nandc->reg_read_buf;
	nandc_read_buffer_sync(nandc, true);

	for (i = 0; i < ecc->steps; i++, buf++) {
		u32 flash, buffer, erased_cw;
		int data_len, oob_len;

		if (qcom_nandc_is_last_cw(ecc, i)) {
			data_len = ecc->size - ((ecc->steps - 1) << 2);
			oob_len = ecc->steps << 2;
		} else {
			data_len = host->cw_data;
			oob_len = 0;
		}

		flash = le32_to_cpu(buf->flash);
		buffer = le32_to_cpu(buf->buffer);
		erased_cw = le32_to_cpu(buf->erased_cw);

		/*
		 * Check ECC failure for each codeword. ECC failure can
		 * happen in either of the following conditions
		 * 1. If number of bitflips are greater than ECC engine
		 *    capability.
		 * 2. If this codeword contains all 0xff for which erased
		 *    codeword detection check will be done.
		 */
		if ((flash & FS_OP_ERR) && (buffer & BS_UNCORRECTABLE_BIT)) {
			/*
			 * For BCH ECC, ignore erased codeword errors, if
			 * ERASED_CW bits are set.
			 */
			if (host->bch_enabled) {
				erased = (erased_cw & ERASED_CW) == ERASED_CW ?
					 true : false;
			/*
			 * For RS ECC, HW reports the erased CW by placing
			 * special characters at certain offsets in the buffer.
			 * These special characters will be valid only if
			 * complete page is read i.e. data_buf is not NULL.
			 */
			} else if (data_buf) {
				erased = erased_chunk_check_and_fixup(data_buf,
								      data_len);
			} else {
				erased = false;
			}

			if (!erased)
				uncorrectable_cws |= BIT(i);
		/*
		 * Check if MPU or any other operational error (timeout,
		 * device failure, etc.) happened for this codeword and
		 * make flash_op_err true. If flash_op_err is set, then
		 * EIO will be returned for page read.
		 */
		} else if (flash & (FS_OP_ERR | FS_MPU_ERR)) {
			flash_op_err = true;
		/*
		 * No ECC or operational errors happened. Check the number of
		 * bits corrected and update the ecc_stats.corrected.
		 */
		} else {
			unsigned int stat;

			stat = buffer & BS_CORRECTABLE_ERR_MSK;
			mtd->ecc_stats.corrected += stat;
			max_bitflips = max(max_bitflips, stat);
		}

		if (data_buf)
			data_buf += data_len;
		if (oob_buf)
			oob_buf += oob_len + ecc->bytes;
	}

	if (flash_op_err)
		return -EIO;

	if (!uncorrectable_cws)
		return max_bitflips;

	return check_for_erased_page(host, data_buf_start, oob_buf_start,
				     uncorrectable_cws, page,
				     max_bitflips);
}

/*
 * helper to perform the actual page read operation, used by ecc->read_page(),
 * ecc->read_oob()
 */
static int read_page_ecc(struct qcom_nand_host *host, u8 *data_buf,
			 u8 *oob_buf, int page)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	u8 *data_buf_start = data_buf, *oob_buf_start = oob_buf;
	int i, ret;
	__le32 *status_buf_start = nandc->status_buf;
	__le32 *status_buf_cw = nandc->status_buf;

	config_nand_page_read(nandc);

	/* queue cmd descs for each codeword */
	for (i = 0; i < ecc->steps; i++) {
		int data_size, oob_size;

		if (qcom_nandc_is_last_cw(ecc, i)) {
			data_size = ecc->size - ((ecc->steps - 1) << 2);
			oob_size = (ecc->steps << 2) + host->ecc_bytes_hw +
				   host->spare_bytes;
		} else {
			data_size = host->cw_data;
			oob_size = host->ecc_bytes_hw + host->spare_bytes;
		}

		if (nandc->props->is_bam) {
			if (data_buf && oob_buf) {
				nandc_set_read_loc(chip, i, 0, 0, data_size, 0);
				nandc_set_read_loc(chip, i, 1, data_size, oob_size, 1);
			} else if (data_buf) {
				nandc_set_read_loc(chip, i, 0, 0, data_size, 1);
			} else {
				nandc_set_read_loc(chip, i, 0, data_size, oob_size, 1);
			}
		}

		config_nand_cw_read(chip, true, i);

		if (data_buf)
			read_data_dma(nandc, FLASH_BUF_ACC, data_buf,
				      data_size, 0);

		if (nandc->props->qpic_v2 && nandc->props->page_scope) {
			read_status_data_dma(nandc, FLASH_BUF_ACC, (void *)status_buf_cw,
					MAX_STATUS_REG, 0);
			status_buf_cw += (MAX_STATUS_REG / sizeof(u32));
		}

		/*
		 * when ecc is enabled, the controller doesn't read the real
		 * or dummy bad block markers in each chunk. To maintain a
		 * consistent layout across RAW and ECC reads, we just
		 * leave the real/dummy BBM offsets empty (i.e, filled with
		 * 0xffs)
		 */
		if (oob_buf) {
			int j;

			for (j = 0; j < host->bbm_size; j++)
				*oob_buf++ = 0xff;

			read_data_dma(nandc, FLASH_BUF_ACC + data_size,
				      oob_buf, oob_size, 0);
		}

		if (data_buf)
			data_buf += data_size;
		if (oob_buf)
			oob_buf += oob_size;
	}

	ret = submit_descs(nandc);
	free_descs(nandc);

	if (ret) {
		dev_err(nandc->dev, "failure to read page/oob\n");
		return ret;
	}
	if (nandc->props->qpic_v2 && nandc->props->page_scope)
		memmove(nandc->reg_read_buf, status_buf_start, nandc->sts_buf_size);

	return parse_read_errors(host, data_buf_start, oob_buf_start, page);
}

/*
 * a helper that copies the last step/codeword of a page (containing free oob)
 * into our local buffer
 */
static int copy_last_cw(struct qcom_nand_host *host, int page)
{
	struct nand_chip *chip = &host->chip;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	int size;
	int ret;

	clear_read_regs(nandc);

	size = host->use_ecc ? host->cw_data : host->cw_size;

	/* prepare a clean read buffer */
	memset(nandc->data_buffer, 0xff, size);

	set_address(host, host->cw_size * (ecc->steps - 1), page);
	update_rw_regs(host, 1, true, ecc->steps - 1);

	config_nand_single_cw_page_read(chip, host->use_ecc, ecc->steps - 1);

	read_data_dma(nandc, FLASH_BUF_ACC, nandc->data_buffer, size, 0);

	ret = submit_descs(nandc);
	if (ret)
		dev_err(nandc->dev, "failed to copy last codeword\n");

	free_descs(nandc);

	return ret;
}

/* implements ecc->read_page() */
static int qcom_nandc_read_page(struct nand_chip *chip, uint8_t *buf,
				int oob_required, int page)
{
	struct qcom_nand_host *host = to_qcom_nand_host(chip);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	u8 *data_buf, *oob_buf = NULL;

	nand_read_page_op(chip, page, 0, NULL, 0);
	data_buf = buf;
	oob_buf = oob_required ? chip->oob_poi : NULL;

	clear_bam_transaction(nandc);

	return read_page_ecc(host, data_buf, oob_buf, page);
}

/* implements ecc->read_page_raw() */
static int qcom_nandc_read_page_raw(struct nand_chip *chip, uint8_t *buf,
				    int oob_required, int page)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct qcom_nand_host *host = to_qcom_nand_host(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	int cw, ret;
	u8 *data_buf = buf, *oob_buf = chip->oob_poi;

	for (cw = 0; cw < ecc->steps; cw++) {
		ret = qcom_nandc_read_cw_raw(mtd, chip, data_buf, oob_buf,
					     page, cw);
		if (ret)
			return ret;

		data_buf += host->cw_data;
		oob_buf += ecc->bytes;
	}

	return 0;
}

/* implements ecc->read_oob() */
static int qcom_nandc_read_oob(struct nand_chip *chip, int page)
{
	struct qcom_nand_host *host = to_qcom_nand_host(chip);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;

	clear_read_regs(nandc);
	clear_bam_transaction(nandc);

	host->use_ecc = true;
	set_address(host, 0, page);
	update_rw_regs(host, ecc->steps, true, 0);

	return read_page_ecc(host, NULL, chip->oob_poi, page);
}

/* implements ecc->write_page() */
static int qcom_nandc_write_page(struct nand_chip *chip, const uint8_t *buf,
				 int oob_required, int page)
{
	struct qcom_nand_host *host = to_qcom_nand_host(chip);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	u8 *data_buf, *oob_buf;
	int i, ret;

	nand_prog_page_begin_op(chip, page, 0, NULL, 0);

	clear_read_regs(nandc);
	clear_bam_transaction(nandc);

	data_buf = (u8 *)buf;
	oob_buf = chip->oob_poi;

	host->use_ecc = true;
	update_rw_regs(host, ecc->steps, false, 0);
	config_nand_page_write(chip);

	for (i = 0; i < ecc->steps; i++) {
		int data_size, oob_size;

		if (qcom_nandc_is_last_cw(ecc, i)) {
			data_size = ecc->size - ((ecc->steps - 1) << 2);
			oob_size = (ecc->steps << 2) + host->ecc_bytes_hw +
				   host->spare_bytes;
		} else {
			data_size = host->cw_data;
			oob_size = ecc->bytes;
		}


		write_data_dma(nandc, FLASH_BUF_ACC, data_buf, data_size,
			       i == (ecc->steps - 1) ? NAND_BAM_NO_EOT : 0);

		/*
		 * when ECC is enabled, we don't really need to write anything
		 * to oob for the first n - 1 codewords since these oob regions
		 * just contain ECC bytes that's written by the controller
		 * itself. For the last codeword, we skip the bbm positions and
		 * write to the free oob area.
		 */
		if (qcom_nandc_is_last_cw(ecc, i)) {
			oob_buf += host->bbm_size;

			write_data_dma(nandc, FLASH_BUF_ACC + data_size,
				       oob_buf, oob_size, 0);
		}

		config_nand_cw_write(chip);

		data_buf += data_size;
		oob_buf += oob_size;
	}

	ret = submit_descs(nandc);
	if (ret)
		dev_err(nandc->dev, "failure to write page\n");

	free_descs(nandc);

	if (!ret)
		ret = nand_prog_page_end_op(chip);

	return ret;
}

/* implements ecc->write_page_raw() */
static int qcom_nandc_write_page_raw(struct nand_chip *chip,
				     const uint8_t *buf, int oob_required,
				     int page)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct qcom_nand_host *host = to_qcom_nand_host(chip);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	u8 *data_buf, *oob_buf;
	int i, ret;

	nand_prog_page_begin_op(chip, page, 0, NULL, 0);
	clear_read_regs(nandc);
	clear_bam_transaction(nandc);

	data_buf = (u8 *)buf;
	oob_buf = chip->oob_poi;

	host->use_ecc = false;
	update_rw_regs(host, ecc->steps, false, 0);
	config_nand_page_write(chip);

	for (i = 0; i < ecc->steps; i++) {
		int data_size1, data_size2, oob_size1, oob_size2;
		int reg_off = FLASH_BUF_ACC;

		data_size1 = mtd->writesize - host->cw_size * (ecc->steps - 1);
		oob_size1 = host->bbm_size;

		if (qcom_nandc_is_last_cw(ecc, i)) {
			data_size2 = ecc->size - data_size1 -
				     ((ecc->steps - 1) << 2);
			oob_size2 = (ecc->steps << 2) + host->ecc_bytes_hw +
				    host->spare_bytes;
		} else {
			data_size2 = host->cw_data - data_size1;
			oob_size2 = host->ecc_bytes_hw + host->spare_bytes;
		}

		write_data_dma(nandc, reg_off, data_buf, data_size1,
			       NAND_BAM_NO_EOT);
		reg_off += data_size1;
		data_buf += data_size1;

		write_data_dma(nandc, reg_off, oob_buf, oob_size1,
			       NAND_BAM_NO_EOT);
		reg_off += oob_size1;
		oob_buf += oob_size1;

		write_data_dma(nandc, reg_off, data_buf, data_size2,
			       NAND_BAM_NO_EOT);
		reg_off += data_size2;
		data_buf += data_size2;

		write_data_dma(nandc, reg_off, oob_buf, oob_size2, 0);
		oob_buf += oob_size2;

		config_nand_cw_write(chip);
	}

	ret = submit_descs(nandc);
	if (ret)
		dev_err(nandc->dev, "failure to write raw page\n");

	free_descs(nandc);

	if (!ret)
		ret = nand_prog_page_end_op(chip);

	return ret;
}

/*
 * implements ecc->write_oob()
 *
 * the NAND controller cannot write only data or only OOB within a codeword
 * since ECC is calculated for the combined codeword. So update the OOB from
 * chip->oob_poi, and pad the data area with OxFF before writing.
 */
static int qcom_nandc_write_oob(struct nand_chip *chip, int page)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct qcom_nand_host *host = to_qcom_nand_host(chip);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	u8 *oob = chip->oob_poi;
	int data_size, oob_size;
	int ret;

	host->use_ecc = true;
	clear_bam_transaction(nandc);

	/* calculate the data and oob size for the last codeword/step */
	data_size = ecc->size - ((ecc->steps - 1) << 2);
	oob_size = mtd->oobavail;

	memset(nandc->data_buffer, 0xff, host->cw_data);
	/* override new oob content to last codeword */
	mtd_ooblayout_get_databytes(mtd, nandc->data_buffer + data_size, oob,
				    0, mtd->oobavail);

	set_address(host, host->cw_size * (ecc->steps - 1), page);
	update_rw_regs(host, 1, false, 0);

	config_nand_page_write(chip);
	write_data_dma(nandc, FLASH_BUF_ACC,
		       nandc->data_buffer, data_size + oob_size, 0);
	config_nand_cw_write(chip);

	ret = submit_descs(nandc);

	free_descs(nandc);

	if (ret) {
		dev_err(nandc->dev, "failure to write oob\n");
		return -EIO;
	}

	return nand_prog_page_end_op(chip);
}

static int qcom_nandc_block_bad(struct nand_chip *chip, loff_t ofs)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct qcom_nand_host *host = to_qcom_nand_host(chip);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	int page, ret, bbpos, bad = 0;

	page = (int)(ofs >> chip->page_shift) & chip->pagemask;

	/*
	 * configure registers for a raw sub page read, the address is set to
	 * the beginning of the last codeword, we don't care about reading ecc
	 * portion of oob. we just want the first few bytes from this codeword
	 * that contains the BBM
	 */
	host->use_ecc = false;

	clear_bam_transaction(nandc);
	ret = copy_last_cw(host, page);
	if (ret)
		goto err;

	if (check_flash_errors(host, 1)) {
		dev_warn(nandc->dev, "error when trying to read BBM\n");
		goto err;
	}

	bbpos = mtd->writesize - host->cw_size * (ecc->steps - 1);

	bad = nandc->data_buffer[bbpos] != 0xff;

	if (chip->options & NAND_BUSWIDTH_16)
		bad = bad || (nandc->data_buffer[bbpos + 1] != 0xff);
err:
	return bad;
}

static int qcom_nandc_block_markbad(struct nand_chip *chip, loff_t ofs)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct qcom_nand_host *host = to_qcom_nand_host(chip);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	int page, ret, bbpos;

	clear_read_regs(nandc);
	clear_bam_transaction(nandc);

	/*
	 * to mark the BBM as bad, we flash the entire last codeword with 0s.
	 * we don't care about the rest of the content in the codeword since
	 * we aren't going to use this block again
	 */
	memset(nandc->data_buffer, 0x00, host->cw_size);

	bbpos = mtd->writesize - host->cw_size * (ecc->steps - 1);
	nandc->data_buffer[bbpos] = SW_BAD_BLOCK_INDICATION;

	page = (int)(ofs >> chip->page_shift) & chip->pagemask;

	/* prepare write */
	host->use_ecc = false;
	set_address(host, host->cw_size * (ecc->steps - 1), page);
	update_rw_regs(host, 1, false, ecc->steps - 1);

	config_nand_page_write(chip);
	write_data_dma(nandc, FLASH_BUF_ACC,
		       nandc->data_buffer, host->cw_size, 0);
	config_nand_cw_write(chip);

	ret = submit_descs(nandc);

	free_descs(nandc);

	if (ret) {
		dev_err(nandc->dev, "failure to update BBM\n");
		return -EIO;
	}

	return nand_prog_page_end_op(chip);
}

/*
 * the three functions below implement chip->legacy.read_byte(),
 * chip->legacy.read_buf() and chip->legacy.write_buf() respectively. these
 * aren't used for reading/writing page data, they are used for smaller data
 * like reading	id, status etc
 */
static uint8_t qcom_nandc_read_byte(struct nand_chip *chip)
{
	struct qcom_nand_host *host = to_qcom_nand_host(chip);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	u8 *buf = nandc->data_buffer;
	u8 ret = 0x0;

	if (host->last_command == NAND_CMD_STATUS) {
		ret = host->status;

		host->status = NAND_STATUS_READY | NAND_STATUS_WP;

		return ret;
	}

	if (nandc->buf_start < nandc->buf_count)
		ret = buf[nandc->buf_start++];

	return ret;
}

static void qcom_nandc_read_buf(struct nand_chip *chip, uint8_t *buf, int len)
{
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	int real_len = min_t(size_t, len, nandc->buf_count - nandc->buf_start);

	memcpy(buf, nandc->data_buffer + nandc->buf_start, real_len);
	nandc->buf_start += real_len;
}

static void qcom_nandc_write_buf(struct nand_chip *chip, const uint8_t *buf,
				 int len)
{
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	int real_len = min_t(size_t, len, nandc->buf_count - nandc->buf_start);

	memcpy(nandc->data_buffer + nandc->buf_start, buf, real_len);

	nandc->buf_start += real_len;
}

/* we support only one external chip for now */
static void qcom_nandc_select_chip(struct nand_chip *chip, int chipnr)
{
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);

	if (chipnr <= 0)
		return;

	dev_warn(nandc->dev, "invalid chip select\n");
}

/*
 * NAND controller page layout info
 *
 * Layout with ECC enabled:
 *
 * |----------------------|  |---------------------------------|
 * |           xx.......yy|  |             *********xx.......yy|
 * |    DATA   xx..ECC..yy|  |    DATA     **SPARE**xx..ECC..yy|
 * |   (516)   xx.......yy|  |  (516-n*4)  **(n*4)**xx.......yy|
 * |           xx.......yy|  |             *********xx.......yy|
 * |----------------------|  |---------------------------------|
 *     codeword 1,2..n-1                  codeword n
 *  <---(528/532 Bytes)-->    <-------(528/532 Bytes)--------->
 *
 * n = Number of codewords in the page
 * . = ECC bytes
 * * = Spare/free bytes
 * x = Unused byte(s)
 * y = Reserved byte(s)
 *
 * 2K page: n = 4, spare = 16 bytes
 * 4K page: n = 8, spare = 32 bytes
 * 8K page: n = 16, spare = 64 bytes
 *
 * the qcom nand controller operates at a sub page/codeword level. each
 * codeword is 528 and 532 bytes for 4 bit and 8 bit ECC modes respectively.
 * the number of ECC bytes vary based on the ECC strength and the bus width.
 *
 * the first n - 1 codewords contains 516 bytes of user data, the remaining
 * 12/16 bytes consist of ECC and reserved data. The nth codeword contains
 * both user data and spare(oobavail) bytes that sum up to 516 bytes.
 *
 * When we access a page with ECC enabled, the reserved bytes(s) are not
 * accessible at all. When reading, we fill up these unreadable positions
 * with 0xffs. When writing, the controller skips writing the inaccessible
 * bytes.
 *
 * Layout with ECC disabled:
 *
 * |------------------------------|  |---------------------------------------|
 * |         yy          xx.......|  |         bb          *********xx.......|
 * |  DATA1  yy  DATA2   xx..ECC..|  |  DATA1  bb  DATA2   **SPARE**xx..ECC..|
 * | (size1) yy (size2)  xx.......|  | (size1) bb (size2)  **(n*4)**xx.......|
 * |         yy          xx.......|  |         bb          *********xx.......|
 * |------------------------------|  |---------------------------------------|
 *         codeword 1,2..n-1                        codeword n
 *  <-------(528/532 Bytes)------>    <-----------(528/532 Bytes)----------->
 *
 * n = Number of codewords in the page
 * . = ECC bytes
 * * = Spare/free bytes
 * x = Unused byte(s)
 * y = Dummy Bad Bock byte(s)
 * b = Real Bad Block byte(s)
 * size1/size2 = function of codeword size and 'n'
 *
 * when the ECC block is disabled, one reserved byte (or two for 16 bit bus
 * width) is now accessible. For the first n - 1 codewords, these are dummy Bad
 * Block Markers. In the last codeword, this position contains the real BBM
 *
 * In order to have a consistent layout between RAW and ECC modes, we assume
 * the following OOB layout arrangement:
 *
 * |-----------|  |--------------------|
 * |yyxx.......|  |bb*********xx.......|
 * |yyxx..ECC..|  |bb*FREEOOB*xx..ECC..|
 * |yyxx.......|  |bb*********xx.......|
 * |yyxx.......|  |bb*********xx.......|
 * |-----------|  |--------------------|
 *  first n - 1       nth OOB region
 *  OOB regions
 *
 * n = Number of codewords in the page
 * . = ECC bytes
 * * = FREE OOB bytes
 * y = Dummy bad block byte(s) (inaccessible when ECC enabled)
 * x = Unused byte(s)
 * b = Real bad block byte(s) (inaccessible when ECC enabled)
 *
 * This layout is read as is when ECC is disabled. When ECC is enabled, the
 * inaccessible Bad Block byte(s) are ignored when we write to a page/oob,
 * and assumed as 0xffs when we read a page/oob. The ECC, unused and
 * dummy/real bad block bytes are grouped as ecc bytes (i.e, ecc->bytes is
 * the sum of the three).
 */
static int qcom_nand_ooblayout_ecc(struct mtd_info *mtd, int section,
				   struct mtd_oob_region *oobregion)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct qcom_nand_host *host = to_qcom_nand_host(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;

	if (section > 1)
		return -ERANGE;

	if (!section) {
		oobregion->length = (ecc->bytes * (ecc->steps - 1)) +
				    host->bbm_size;
		oobregion->offset = 0;
	} else {
		oobregion->length = host->ecc_bytes_hw + host->spare_bytes;
		oobregion->offset = mtd->oobsize - oobregion->length;
	}

	return 0;
}

static int qcom_nand_ooblayout_free(struct mtd_info *mtd, int section,
				     struct mtd_oob_region *oobregion)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct qcom_nand_host *host = to_qcom_nand_host(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;

	if (section)
		return -ERANGE;

	oobregion->length = ecc->steps * 4;
	oobregion->offset = ((ecc->steps - 1) * ecc->bytes) + host->bbm_size;

	return 0;
}

static const struct mtd_ooblayout_ops qcom_nand_ooblayout_ops = {
	.ecc = qcom_nand_ooblayout_ecc,
	.free = qcom_nand_ooblayout_free,
};

static int
qcom_nandc_calc_ecc_bytes(int step_size, int strength)
{
	return strength == 4 ? 12 : 16;
}
NAND_ECC_CAPS_SINGLE(qcom_nandc_ecc_caps, qcom_nandc_calc_ecc_bytes,
		     NANDC_STEP_SIZE, 4, 8);

static int qcom_nand_attach_chip(struct nand_chip *chip)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct qcom_nand_host *host = to_qcom_nand_host(chip);
	struct nand_ecc_ctrl *ecc = &chip->ecc;
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	int cwperpage, bad_block_byte, ret;
	bool wide_bus;
	int ecc_mode = 1;
	int num_addr_cycle = 5, dsbl_sts_aftr_write = 0;
	int wr_rd_bsy_gap = 2, recovery_cycle = 7;

	/* controller only supports 512 bytes data steps */
	ecc->size = NANDC_STEP_SIZE;
	wide_bus = chip->options & NAND_BUSWIDTH_16 ? true : false;
	cwperpage = mtd->writesize / NANDC_STEP_SIZE;

	/*
	 * Read the required ecc strength from NAND device and overwrite the
	 * device tree ecc strength
	 */
	if (chip->base.eccreq.strength >= 8)
		ecc->strength = 8;
	/*
	 * Each CW has 4 available OOB bytes which will be protected with ECC
	 * so remaining bytes can be used for ECC.
	 */
	ret = nand_ecc_choose_conf(chip, &qcom_nandc_ecc_caps,
				   mtd->oobsize - (cwperpage * 4));
	if (ret) {
		dev_err(nandc->dev, "No valid ECC settings possible\n");
		return ret;
	}

	if (ecc->strength >= 8) {
		/* 8 bit ECC defaults to BCH ECC on all platforms */
		host->bch_enabled = true;
		ecc_mode = 1;

		if (wide_bus) {
			host->ecc_bytes_hw = 14;
			host->spare_bytes = 0;
			host->bbm_size = 2;
		} else {
			host->ecc_bytes_hw = 13;
			host->spare_bytes = 2;
			host->bbm_size = 1;
		}
	} else {
		/*
		 * if the controller supports BCH for 4 bit ECC, the controller
		 * uses lesser bytes for ECC. If RS is used, the ECC bytes is
		 * always 10 bytes
		 */
		if (nandc->props->ecc_modes & ECC_BCH_4BIT) {
			/* BCH */
			host->bch_enabled = true;
			ecc_mode = 0;

			if (wide_bus) {
				host->ecc_bytes_hw = 8;
				host->spare_bytes = 2;
				host->bbm_size = 2;
			} else {
				host->ecc_bytes_hw = 7;
				host->spare_bytes = 4;
				host->bbm_size = 1;
			}
		} else {
			/* RS */
			host->ecc_bytes_hw = 10;

			if (wide_bus) {
				host->spare_bytes = 0;
				host->bbm_size = 2;
			} else {
				host->spare_bytes = 1;
				host->bbm_size = 1;
			}
		}
	}

	/*
	 * we consider ecc->bytes as the sum of all the non-data content in a
	 * step. It gives us a clean representation of the oob area (even if
	 * all the bytes aren't used for ECC).It is always 16 bytes for 8 bit
	 * ECC and 12 bytes for 4 bit ECC
	 */
	ecc->bytes = host->ecc_bytes_hw + host->spare_bytes + host->bbm_size;

	ecc->read_page		= qcom_nandc_read_page;
	ecc->read_page_raw	= qcom_nandc_read_page_raw;
	ecc->read_oob		= qcom_nandc_read_oob;
	ecc->write_page		= qcom_nandc_write_page;
	ecc->write_page_raw	= qcom_nandc_write_page_raw;
	ecc->write_oob		= qcom_nandc_write_oob;

	ecc->mode = NAND_ECC_HW;

	mtd_set_ooblayout(mtd, &qcom_nand_ooblayout_ops);

	/* Free the initially allocated BAM transaction for reading the ONFI params */
	if (nandc->props->is_bam)
		free_bam_transaction(nandc);

	nandc->max_cwperpage = max_t(unsigned int, nandc->max_cwperpage,
				     cwperpage);

	if (nandc->props->is_bam) {
		nandc->bam_txn = alloc_bam_transaction(nandc);
		if (!nandc->bam_txn) {
			dev_err(nandc->dev,
				"failed to allocate bam transaction\n");
			return -ENOMEM;
		}
	}

	/*
	 * DATA_UD_BYTES varies based on whether the read/write command protects
	 * spare data with ECC too. We protect spare data by default, so we set
	 * it to main + spare data, which are 512 and 4 bytes respectively.
	 */
	host->cw_data = 516;

	/*
	 * total bytes in a step, either 528 bytes for 4 bit ECC, or 532 bytes
	 * for 8 bit ECC
	 */
	host->cw_size = host->cw_data + ecc->bytes;
	bad_block_byte = mtd->writesize - host->cw_size * (cwperpage - 1) + 1;

	/* For QSPI serial nand QPIC config register value got changed
	 * so configure the new value for qspi serial nand
	 */
	if (nandc->props->is_serial_nand) {
		num_addr_cycle = 3;
		dsbl_sts_aftr_write = 1;
		wr_rd_bsy_gap = 20;
		recovery_cycle = 0;
	}

	host->cfg0 = (cwperpage - 1) << CW_PER_PAGE
				| host->cw_data << UD_SIZE_BYTES
				| dsbl_sts_aftr_write << DISABLE_STATUS_AFTER_WRITE
				| num_addr_cycle << NUM_ADDR_CYCLES
				| host->ecc_bytes_hw << ECC_PARITY_SIZE_BYTES_RS
				| 0 << STATUS_BFR_READ
				| 1 << SET_RD_MODE_AFTER_STATUS
				| host->spare_bytes << SPARE_SIZE_BYTES;

	host->cfg1 = recovery_cycle << NAND_RECOVERY_CYCLES
				| 0 <<  CS_ACTIVE_BSY
				| bad_block_byte << BAD_BLOCK_BYTE_NUM
				| 0 << BAD_BLOCK_IN_SPARE_AREA
				| wr_rd_bsy_gap << WR_RD_BSY_GAP
				| wide_bus << WIDE_FLASH
				| host->bch_enabled << ENABLE_BCH_ECC;

	host->cfg0_raw = (cwperpage - 1) << CW_PER_PAGE
				| host->cw_size << UD_SIZE_BYTES
				| num_addr_cycle << NUM_ADDR_CYCLES
				| 0 << SPARE_SIZE_BYTES;

	host->cfg1_raw = recovery_cycle << NAND_RECOVERY_CYCLES
				| 0 << CS_ACTIVE_BSY
				| 17 << BAD_BLOCK_BYTE_NUM
				| 1 << BAD_BLOCK_IN_SPARE_AREA
				| wr_rd_bsy_gap << WR_RD_BSY_GAP
				| wide_bus << WIDE_FLASH
				| 1 << DEV0_CFG1_ECC_DISABLE;

	host->ecc_bch_cfg = !host->bch_enabled << ECC_CFG_ECC_DISABLE
				| 0 << ECC_SW_RESET
				| host->cw_data << ECC_NUM_DATA_BYTES
				| 1 << ECC_FORCE_CLK_OPEN
				| ecc_mode << ECC_MODE
				| host->ecc_bytes_hw << ECC_PARITY_SIZE_BYTES_BCH;

	host->ecc_buf_cfg = 0x203 << NUM_STEPS;

	host->clrflashstatus = FS_READY_BSY_N;
	host->clrreadstatus = 0xc0;
	nandc->regs->erased_cw_detect_cfg_clr =
		cpu_to_le32(CLR_ERASED_PAGE_DET);
	nandc->regs->erased_cw_detect_cfg_set =
		cpu_to_le32(SET_ERASED_PAGE_DET);

	dev_dbg(nandc->dev,
		"cfg0 %x cfg1 %x ecc_buf_cfg %x ecc_bch cfg %x cw_size %d cw_data %d strength %d parity_bytes %d steps %d\n",
		host->cfg0, host->cfg1, host->ecc_buf_cfg, host->ecc_bch_cfg,
		host->cw_size, host->cw_data, ecc->strength, ecc->bytes,
		cwperpage);

	return 0;
}

static const struct nand_controller_ops qcom_nandc_ops = {
	.attach_chip = qcom_nand_attach_chip,
};

static int qcom_nandc_alloc(struct qcom_nand_controller *nandc)
{
	int ret;

	ret = dma_set_coherent_mask(nandc->dev, DMA_BIT_MASK(32));
	if (ret) {
		dev_err(nandc->dev, "failed to set DMA mask\n");
		return ret;
	}

	/*
	 * we use the internal buffer for reading ONFI params, reading small
	 * data like ID and status, and preforming read-copy-write operations
	 * when writing to a codeword partially. 532 is the maximum possible
	 * size of a codeword for our nand controller
	 */
	nandc->buf_size = 532;

	nandc->data_buffer = devm_kzalloc(nandc->dev, nandc->buf_size,
					GFP_KERNEL);
	if (!nandc->data_buffer)
		return -ENOMEM;

	nandc->regs = devm_kzalloc(nandc->dev, sizeof(*nandc->regs),
					GFP_KERNEL);
	if (!nandc->regs)
		return -ENOMEM;

	nandc->reg_read_buf = devm_kcalloc(nandc->dev,
				MAX_REG_RD, sizeof(*nandc->reg_read_buf),
				GFP_KERNEL);
	if (!nandc->reg_read_buf)
		return -ENOMEM;

	if (nandc->props->is_bam) {
		nandc->reg_read_dma =
			dma_map_single(nandc->dev, nandc->reg_read_buf,
				       MAX_REG_RD *
				       sizeof(*nandc->reg_read_buf),
				       DMA_FROM_DEVICE);
		if (dma_mapping_error(nandc->dev, nandc->reg_read_dma)) {
			dev_err(nandc->dev, "failed to DMA MAP reg buffer\n");
			return -EIO;
		}

		nandc->tx_chan = dma_request_slave_channel(nandc->dev, "tx");
		if (!nandc->tx_chan) {
			dev_err(nandc->dev, "failed to request tx channel\n");
			return -ENODEV;
		}

		nandc->rx_chan = dma_request_slave_channel(nandc->dev, "rx");
		if (!nandc->rx_chan) {
			dev_err(nandc->dev, "failed to request rx channel\n");
			return -ENODEV;
		}

		nandc->cmd_chan = dma_request_slave_channel(nandc->dev, "cmd");
		if (!nandc->cmd_chan) {
			dev_err(nandc->dev, "failed to request cmd channel\n");
			return -ENODEV;
		}

		if (nandc->props->qpic_v2) {
			nandc->sts_chan = dma_request_slave_channel(nandc->dev, "sts");
			if (!nandc->sts_chan) {
				dev_err(nandc->dev, "failed to request sts channel\n");
				return -ENODEV;
			}
		}

		/*
		 * Initially allocate BAM transaction to read ONFI param page.
		 * After detecting all the devices, this BAM transaction will
		 * be freed and the next BAM tranasction will be allocated with
		 * maximum codeword size
		 */
		nandc->max_cwperpage = 1;
		nandc->bam_txn = alloc_bam_transaction(nandc);
		if (!nandc->bam_txn) {
			dev_err(nandc->dev,
				"failed to allocate bam transaction\n");
			return -ENOMEM;
		}
	} else {
		nandc->chan = dma_request_slave_channel(nandc->dev, "rxtx");
		if (!nandc->chan) {
			dev_err(nandc->dev,
				"failed to request slave channel\n");
			return -ENODEV;
		}
	}

	INIT_LIST_HEAD(&nandc->desc_list);
	INIT_LIST_HEAD(&nandc->host_list);

	nand_controller_init(&nandc->controller);
	nandc->controller.ops = &qcom_nandc_ops;

	return 0;
}

static void qcom_nandc_unalloc(struct qcom_nand_controller *nandc)
{
	if (nandc->props->is_bam) {
		if (!dma_mapping_error(nandc->dev, nandc->reg_read_dma))
			dma_unmap_single(nandc->dev, nandc->reg_read_dma,
					 MAX_REG_RD *
					 sizeof(*nandc->reg_read_buf),
					 DMA_FROM_DEVICE);

		if (nandc->tx_chan)
			dma_release_channel(nandc->tx_chan);

		if (nandc->rx_chan)
			dma_release_channel(nandc->rx_chan);

		if (nandc->cmd_chan)
			dma_release_channel(nandc->cmd_chan);

		if (nandc->props->qpic_v2) {
			if (nandc->sts_chan)
				dma_release_channel(nandc->sts_chan);
		}

	} else {
		if (nandc->chan)
			dma_release_channel(nandc->chan);
	}
}

/* one time setup of a few nand controller registers */
static int qcom_nandc_setup(struct qcom_nand_controller *nandc)
{
	u32 nand_ctrl;

	/* kill onenand */
	if (!nandc->props->is_qpic && !nandc->props->qpic_v2)
		nandc_write(nandc, SFLASHC_BURST_CFG, 0);
	nandc_write(nandc, dev_cmd_reg_addr(nandc, NAND_DEV_CMD_VLD),
		    NAND_DEV_CMD_VLD_VAL);

	/* enable ADM or BAM DMA */
	if (nandc->props->is_bam) {
		nand_ctrl = nandc_read(nandc, NAND_CTRL);
		/*
		 * Once BAM_MODE_EN bit is set then QPIC_NAND_CTRL register
		 * should be written with BAM instead of writel.
		 * Check if BAM_MODE_EN is already set by bootloader and write
		 * only if this bit is not set.
		 */
		if (!(nand_ctrl & BAM_MODE_EN))
			nandc_write(nandc, NAND_CTRL, nand_ctrl | BAM_MODE_EN);
	} else {
		nandc_write(nandc, NAND_FLASH_CHIP_SELECT, DM_EN);
	}

	/* save the original values of these registers */
	nandc->cmd1 = nandc_read(nandc, dev_cmd_reg_addr(nandc, NAND_DEV_CMD1));
	nandc->vld = NAND_DEV_CMD_VLD_VAL;

	return 0;
}

static void qspi_write_reg_bam(struct qcom_nand_controller *nandc,
		unsigned int val, unsigned int reg)
{
	int ret;
	clear_bam_transaction(nandc);
	nandc_set_reg(nandc, reg, val);
	write_reg_dma(nandc, reg, 1, NAND_BAM_NEXT_SGL);

	ret = submit_descs(nandc);
	if (ret)
		dev_err(nandc->dev, "Error in submitting descriptor to write reg %x\n", reg);
	free_descs(nandc);
}

static void qspi_nand_init(struct qcom_nand_controller *nandc)
{
	u32 spi_cfg_val = 0x0;
	u32 reg = 0x0;

	spi_cfg_val |= (LOAD_CLK_CNTR_INIT_EN | (CLK_CNTR_INIT_VAL_VEC << 16)
			| (FEA_STATUS_DEV_ADDR << 8) | SPI_CFG);

	qspi_write_reg_bam(nandc, 0x0, NAND_FLASH_SPI_CFG);
	qspi_write_reg_bam(nandc, spi_cfg_val, NAND_FLASH_SPI_CFG);
	spi_cfg_val &= ~LOAD_CLK_CNTR_INIT_EN;
	qspi_write_reg_bam(nandc, spi_cfg_val, NAND_FLASH_SPI_CFG);

	reg = dev_cmd_reg_addr(nandc, NAND_DEV_CMD0);
	nandc_write(nandc, reg, CMD0_VAL);
	nandc_write(nandc, reg + 4, CMD1_VAL);
	nandc_write(nandc, reg + 8, CMD2_VAL);
	nandc_write(nandc, reg + 12, CMD_VLD_VAL);
	nandc_write(nandc, reg + 16, CMD7_VAL);
	reg = dev_cmd_reg_addr(nandc, NAND_DEV_CMD3);
	nandc_write(nandc, reg, CMD3_VAL);

	qspi_write_reg_bam(nandc, SPI_NUM_ADDR, NAND_SPI_NUM_ADDR_CYCLES);
	qspi_write_reg_bam(nandc, WAIT_CNT, NAND_SPI_BUSY_CHECK_WAIT_CNT);
}

static void qspi_set_phase(struct qcom_nand_controller *nandc, int phase)
{
	u32 qspi_cfg_val = 0x0;
	int reg = dev_cmd_reg_addr(nandc, NAND_FLASH_SPI_CFG);

	qspi_cfg_val = nandc_read(nandc, reg);
	qspi_cfg_val |= LOAD_CLK_CNTR_INIT_EN;

	qspi_write_reg_bam(nandc, qspi_cfg_val, NAND_FLASH_SPI_CFG);
	qspi_cfg_val &= 0xf000ffff;

	/* Write phase value for all the lines */
	qspi_cfg_val |= ((phase << 16) | (phase << 19) | (phase << 22)
			| (phase << 25));
	qspi_write_reg_bam(nandc, qspi_cfg_val, NAND_FLASH_SPI_CFG);

	/* Clear LOAD_CLK_CNTR_INIT_EN bit to load phase value */
	qspi_cfg_val &= ~LOAD_CLK_CNTR_INIT_EN;
	qspi_write_reg_bam(nandc, qspi_cfg_val, NAND_FLASH_SPI_CFG);
}

static int qspi_get_appropriate_phase(struct qcom_nand_controller *nandc, u8 *phase_table,
		int phase_count)
{
	int i, cnt = 0, phase = 0x0;
	u8 phase_ranges[TOTAL_NUM_PHASE] = {'\0'};

	for (i = 0; i < phase_count; i++) {
		if ((phase_table[i] + 1 == phase_table[i + 1]) &&
		(phase_table[i + 1] + 1 == phase_table[i + 2]))
			phase_ranges[cnt++] = phase_table[i + 1];
	}

	/* Filter out middle phase */
	if (cnt == 1)
		phase = phase_table[1];
	if (cnt > 1 && cnt <= TOTAL_NUM_PHASE) {
		if (!(cnt & 1))
			phase = phase_ranges[cnt/2 - 1];
		else
			phase = phase_ranges[cnt/2];
	}

	return phase;
}

static int qpic_serial_check_status(__le32 *status)
{
	u32 flash  = le32_to_cpu(*(__le32 *)status);

	if (flash & FLASH_ERROR) {
		if (flash & FS_MPU_ERR)
			return -EPERM;
		if (flash & FS_TIMEOUT_ERR)
			return -ETIMEDOUT;
		if (flash & FS_OP_ERR)
			return -EIO;
	}
	return 0;
}

static void qcom_check_quad_mode(struct mtd_info *mtd, struct qcom_nand_host *host)
{
	int i, ret;
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	unsigned int command = NAND_CMD_READID_SERIAL;
	u8 id_data[3];
	u32 cmd3_val;

	pre_command(host, command);

	/* get the device id from device */
	nandc->buf_count = 4;
	read_id(host, 0x00);

	ret = submit_descs(nandc);
	if (ret)
		dev_err(nandc->dev,
				"failure submitting descs for command %d\n",
				command);
	free_descs(nandc);

	post_command(host, command);

	/* Read Id bytes */
	for (i = 0; i < 3; i++)
		id_data[i] = chip->legacy.read_byte(chip);
	if (id_data[0] == SPI_FLASH_MICRON_ID) {
		cmd3_val = CMD3_VAL & CMD3_MASK;
		host->check_qe_bit = false;
		nandc_write(nandc, dev_cmd_reg_addr(nandc, NAND_DEV_CMD3), cmd3_val);
	} else if (id_data[0] == SPI_FLASH_GIGA_ID &&
			id_data[1] == SPI_FLASH_ESMT_DEVICE_ID) {
	       host->check_qe_bit = false;
	} else if (id_data[0] == SPI_FLASH_WINBOND_ID &&
			id_data[1] == SPI_WINBOND_DEVICE_1) {
	       host->check_qe_bit = false;
	} else
		host->check_qe_bit = true;
}

static int qcom_serial_get_feature(struct qcom_nand_controller *nandc,
		struct qcom_nand_host *host, u32 faddr)
{
	u32 cmd_val = 0x0;
	u32 command = NAND_CMD_GET_FEATURE_SERIAL;
	int ret;

	/* Clear the BAM transaction index */
	clear_bam_transaction(nandc);

	cmd_val = (SPI_TRANSFER_MODE_x1 | SPI_WP | SPI_HOLD |
			ACC_FEATURE);

	pre_command(host, command);

	nandc_set_reg(nandc, NAND_FLASH_CMD, cmd_val);
	nandc_set_reg(nandc, NAND_ADDR0, faddr);
	nandc_set_reg(nandc, NAND_ADDR1, 0);

	/* Clear the feature register value to get correct feature value */
	nandc_set_reg(nandc, NAND_FLASH_FEATURES, 0);

	nandc_set_reg(nandc, NAND_EXEC_CMD, 1);

	write_reg_dma(nandc, NAND_FLASH_CMD, 3, NAND_BAM_NEXT_SGL);

	write_reg_dma(nandc, NAND_FLASH_FEATURES, 1, NAND_BAM_NEXT_SGL);

	write_reg_dma(nandc, NAND_EXEC_CMD, 1, NAND_BAM_NEXT_SGL);

	read_reg_dma(nandc, NAND_FLASH_FEATURES, 1, NAND_BAM_NEXT_SGL);
	/* submit the descriptor to bam for execution*/
	ret = submit_descs(nandc);
	free_descs(nandc);
	if (ret) {
		dev_err(nandc->dev, "Error in submitting descriptor for command:%d\n",
				command);
		return ret;
	}

	nandc_read_buffer_sync(nandc, true);

	/* read_reg_dma will read data in to nandc->reg_read_buf
	 * so after issueing command in read_reg_dma function read reg_read_buf
	 * buffer
	 */
	ret = le32_to_cpu(*(__le32 *)nandc->reg_read_buf);

	return ret;
}

static int qcom_serial_set_feature(struct qcom_nand_controller *nandc,
		struct qcom_nand_host *host, u32 faddr, u32 fval)
{
	int ret;
	u32 command = NAND_CMD_SET_FEATURE_SERIAL;
	u32 cmd_val = (SPI_TRANSFER_MODE_x1 | SPI_WP | SPI_HOLD |
			ACC_FEATURE | QPIC_SET_FEATURE);

	/* Clear the BAM transaction index */
	clear_bam_transaction(nandc);

	pre_command(host, command);

	nandc_set_reg(nandc, NAND_FLASH_CMD, cmd_val);
	nandc_set_reg(nandc, NAND_ADDR0, faddr);
	nandc_set_reg(nandc, NAND_ADDR1, 0);
	nandc_set_reg(nandc, NAND_FLASH_FEATURES, fval);

	nandc_set_reg(nandc, NAND_EXEC_CMD, 1);

	write_reg_dma(nandc, NAND_FLASH_CMD, 3, NAND_BAM_NEXT_SGL);

	write_reg_dma(nandc, NAND_FLASH_FEATURES, 1, NAND_BAM_NEXT_SGL);

	write_reg_dma(nandc, NAND_EXEC_CMD, 1, NAND_BAM_NEXT_SGL);

	read_reg_dma(nandc, NAND_FLASH_STATUS, 1, NAND_BAM_NEXT_SGL);

	/* submit the descriptor to bam for execution*/
	ret = submit_descs(nandc);
	free_descs(nandc);
	if (ret) {
		dev_err(nandc->dev, "Error in submitting descriptor for command:%d\n",
				command);
		return ret;
	}

	/* read_reg_dma will read data in to nandc->reg_read_buf
	 * so after issueing command in read_reg_dma function read reg_read_buf
	 * buffer
	 */
	nandc_read_buffer_sync(nandc, true);

	ret = qpic_serial_check_status(nandc->reg_read_buf);
	if (ret) {
		dev_err(nandc->dev, "Error in executing command:%d\n",command);
		return ret;
	}
	return ret;
}


static bool config_buf_bit(struct mtd_info *mtd, struct qcom_nand_host *host, u8 *pos)
{
	int i, ret;
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct qcom_nand_controller *nandc = get_qcom_nand_controller(chip);
	unsigned int command = NAND_CMD_READID_SERIAL;
	u8 id_data[3];

	pre_command(host, command);

	/* get the device id from device */
	nandc->buf_count = 4;
	read_id(host, 0x00);

	ret = submit_descs(nandc);
	if (ret)
		dev_err(nandc->dev, "failure submitting descs for command %d\n",
				command);
	free_descs(nandc);

	post_command(host, command);

	/* Read Id bytes */
	for (i = 0; i < 2; i++)
		id_data[i] = chip->legacy.read_byte(chip);
/* Add device ID here if SPI Nand supports BUF_BIT to configure */
	switch (id_data[0]) {
	case SPI_FLASH_WINBOND_ID:
		*pos = 3;
		return true;
	default:
		return false;
	}
}

static int qspi_nand_device_config(struct qcom_nand_controller *nandc,
				   struct qcom_nand_host *host, struct mtd_info *mtd)
{
	int status = 0;
	u8 buf_bit_pos = 0;
	nandc->buf_count = 4;
	memset(nandc->reg_read_buf, 0x0, nandc->buf_count);
	/* Configure BUF bit for SPI Nand device
	 * Read the id and compare for device id
	 */
	if (config_buf_bit(mtd, host, &buf_bit_pos)) {
		status = qcom_serial_get_feature(nandc, host, SPI_FLASH_FEATURE_REG);
		if (status < 0) {
			dev_err(nandc->dev,"Error in getting feature Continous buff");
			return status;
		}

		if (!((status >> 8) & SPI_NAND_BUF_BIT(buf_bit_pos))) {
			dev_dbg(nandc->dev, "Continous buffer mode not enabled on power on\n");
			dev_dbg(nandc->dev, "Issuing set feature command enbale it\n");
			status = qcom_serial_set_feature(nandc, host, SPI_FLASH_FEATURE_REG,
					SPI_NAND_BUF_BIT(buf_bit_pos) | (status >> 8));
			if (status < 0) {
				dev_err(nandc->dev,"Error in setting feature Quad mode.");
				return status;
			}
		} else {
			dev_dbg(nandc->dev, "Continous buffer mode enabled on power on\n");
		}
	}

	qcom_check_quad_mode(mtd, host);

	if (!host->check_qe_bit) {
		host->quad_mode = true;
		return 0;
	}

	if (nandc->props->quad_mode) {
		/* Check if device supports x4 Mode and enable it if not enabled*/
		status = qcom_serial_get_feature(nandc, host,
							SPI_FLASH_FEATURE_REG);
		if (status < 0) {
			dev_err(nandc->dev, "Error in getting feature x4 mode\n");
			return status;
		}

		if (!((status >> 8) & SPI_FLASH_QUAD_MODE)) {
			/* If x4 mode bit not enabled issue set feature command
			 * to enable quad mode bit of flash device.
			 */
			status = qcom_serial_set_feature(nandc, host,
							SPI_FLASH_FEATURE_REG,
							SPI_FLASH_QUAD_MODE);
			if (status < 0) {
				dev_err(nandc->dev, "Error in setting feature x4 mode\n");
				return status;
			}
			/* again issue the get feature command to check if quad
			 * mode is enabled or not
			 */
			status = qcom_serial_get_feature(nandc, host,
							SPI_FLASH_FEATURE_REG);
			if (status < 0) {
				dev_err(nandc->dev, "Error in getting feature x4 mode\n");
				return status;
			}

			if ((status >> 8) & SPI_FLASH_QUAD_MODE) {
				host->quad_mode = true;
				dev_info(nandc->dev, "x4 mode enabled successfully\n");
			} else {
				host->quad_mode = false;
				dev_err(nandc->dev, "x4 mode not enabled, using x1 mode\n");
				return 0;
			}
		} else {
			dev_info(nandc->dev, "x4 mode enabled already remotely\n");
			host->quad_mode = true;
		}
	}
	return 0;
}

static int qspi_execute_training(struct qcom_nand_controller *nandc,
		struct qcom_nand_host *host, struct mtd_info *mtd)
{
	u32 pages_per_block = 0, page = 0;
	int ret = 0, bb_cnt = 0, i, phase_failed = 0;
	int phase_cnt, phase;
	u32 training_offset = 0;
	u8 *training_data = NULL, trained_phase[TOTAL_NUM_PHASE] = {'\0'};
	struct nand_chip *chip = &host->chip;
	int reg;

	u32 max_iomacro_clk = 0;
	struct device_node *np = nandc->dev->of_node;
	int sz;
	u32 *arr = NULL;
	u32 len = 0;

	/* Set feedback clk enable bit to do auto adjustment of phase
	 * at lower frequency
	 */
	reg = dev_cmd_reg_addr(nandc, NAND_QSPI_MSTR_CONFIG);
	qspi_write_reg_bam(nandc, (nandc_read(nandc,
			reg) | FEEDBACK_CLK_EN),
			NAND_QSPI_MSTR_CONFIG);

	/* Read the training offset patched from u-boot */
	if (of_property_read_u32(np, "qcom,training_offset",
				&training_offset)) {
		dev_err(nandc->dev, "Serial training partition not found");
		ret = -EINVAL;
		goto trng_err;
	}

	pages_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
	page = (training_offset >> chip->page_shift) & chip->pagemask;

	/* check for bad block in allocated training blocks
	 * The training blocks should be continuous good block or
	 * continuous bad block, it should be not like good,bad,good etc.
	 * avoid to use this type of block for serial training
	 */
	while(qcom_nandc_block_bad(chip, training_offset) && bb_cnt < MAX_TRAINING_BLK) {
		training_offset += mtd->erasesize;
		page += pages_per_block;
		bb_cnt++;
	}

	if (bb_cnt == MAX_TRAINING_BLK) {
		dev_dbg(nandc->dev, "All training blocks are bad, skipping serial training");
		dev_dbg(nandc->dev, "Operatig at lower frequency");
		ret = -EINVAL;
		goto trng_err;
	}

	qcom_nandc_command(chip, NAND_CMD_ERASE1, 0, page);

	/* Allocate memory to hold one NAND page */
	training_data = kzalloc(mtd->writesize, GFP_KERNEL);
	if (!training_data) {
		dev_err(nandc->dev, "Error in allocating memory");
		ret = -ENOMEM;
		goto trng_err;
	}
	memset(training_data, '\0', mtd->writesize);

	for (i = 0; i < mtd->writesize; i += sizeof(qspi_training_block_64))
		memcpy(training_data + i, qspi_training_block_64,
			sizeof(qspi_training_block_64));

	/* Write qspi training data to flash */
	ret = qcom_nandc_write_page(chip, training_data, 0, page);
	if (ret) {
		dev_err(nandc->dev, "Error in writing training data");
		ret = -EINVAL;
		goto mem_err;
	}

	/* Read qspi training data @ low freq */
	memset(training_data, 0xff, mtd->writesize);
	ret = qcom_nandc_read_page(chip, training_data, 0, page);
	if (ret < 0) {
		dev_err(nandc->dev, "Error in reading training data @ low freq");
		ret = -EINVAL;
		goto mem_err;
	}

	/* compare read training data with known pattern */
	for (i = 0; i <  mtd->writesize; i += sizeof(qspi_training_block_64)) {
		if (memcmp(training_data + i, qspi_training_block_64,
				sizeof(qspi_training_block_64))) {
			dev_err(nandc->dev, "Training data mismatch @ low freq");
			ret = -EINVAL;
			goto mem_err;
		}
	}

	/* clear feedback clock bit and start training here */
	qspi_write_reg_bam(nandc, (nandc_read(nandc,
			reg) & ~FEEDBACK_CLK_EN),
			NAND_QSPI_MSTR_CONFIG);

	/* Get max io macro clock from device tree, value should be
	 * 200 MHz, 380 MHz, 400 MHz etc.
	 * */
	if (of_property_read_u32(np, "qcom,io_macro_max_clk",
				&max_iomacro_clk)) {
		dev_err(nandc->dev, "Error in reading max io macro clock from dts");
		goto mem_err;
	}

	/* Read all supported io_macro clock frequency from dts */
	if (!of_get_property(np, "qcom,io_macro_clk_rates", &len)) {
		dev_err(nandc->dev, "Error in reading length of io_macro_clock\n");
		goto mem_err;
	}

	sz = (len / sizeof(*arr));

	arr = kzalloc(sz * sizeof(*arr), GFP_KERNEL);
	if (!arr) {
		dev_err(nandc->dev, "failed allocating memory for qcom,io_macro_clk_rates\n");
		goto mem_err;
	}

	ret = of_property_read_u32_array(np, "qcom,io_macro_clk_rates", arr, sz);
	if (ret < 0) {
		dev_err(nandc->dev, "failed reading array qcom,io_macro_clk_rates %d\n", ret);
		goto mem_err;
	}

	sz -= 1;

iomacro_set_clk:
	ret =  clk_set_rate(nandc->iomacro_clk, max_iomacro_clk);
	if (ret) {
		dev_err(nandc->dev,"Setting clk rate to %d MHz failed", max_iomacro_clk);
		goto mem_err;
	}

	phase = 1;
	phase_cnt = 0;

	do {
		qspi_set_phase(nandc, phase);

		/* Prepare clean buffer to read */
		memset(training_data, 0xff, mtd->writesize);
		ret = qcom_nandc_read_page(chip, training_data, 0, page);
		if (ret < 0) {
			dev_err(nandc->dev, "Error in reading training page at %d MHz",
					max_iomacro_clk);
			/* Fall back to next lower clock */
			if (sz < 0)
				goto default_freq;
			max_iomacro_clk = arr[--sz];
			goto iomacro_set_clk;
		}
		/* compare read training data with known pattern */
		for (i = 0; i <  mtd->writesize; i += sizeof(qspi_training_block_64)) {
			if (memcmp(training_data + i, qspi_training_block_64,
					sizeof(qspi_training_block_64))) {
				phase_failed++;
				break;
			}
		}

		if (i == mtd->writesize)
			trained_phase[phase_cnt++] = phase;

	} while (phase++ < TOTAL_NUM_PHASE);

	if (phase_cnt) {
		phase = qspi_get_appropriate_phase(nandc, trained_phase, phase_cnt);
		if (phase == 0) {
			dev_err(nandc->dev, "No continous three phase found at %d MHz",
					max_iomacro_clk);
			if (sz < 0)
				goto default_freq;
			max_iomacro_clk = arr[--sz];
			goto iomacro_set_clk;
		}
		qspi_set_phase(nandc, phase);
	} else {
default_freq:
		dev_err(nandc->dev,"Serial training failed");
		dev_err(nandc->dev, "Running @ low freq 50MHz");
		/* Run @ lower frequency 50Mhz with feedback clk bit enabled  */
		qspi_write_reg_bam(nandc, (nandc_read(nandc,
			reg) | FEEDBACK_CLK_EN),
			NAND_QSPI_MSTR_CONFIG);
		ret =  clk_set_rate(nandc->iomacro_clk, IO_MACRO_50_MHZ);
		if (ret) {
			dev_err(nandc->dev,"Setting clk rate to 50000000 MHz failed");
			goto mem_err;
		}
	}

mem_err:
	kfree(training_data);
trng_err:
	if (arr)
		kfree(arr);
	return ret;
}

static int qcom_nand_host_init_and_register(struct qcom_nand_controller *nandc,
					    struct qcom_nand_host *host,
					    struct device_node *dn)
{
	struct nand_chip *chip = &host->chip;
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct device *dev = nandc->dev;
	int ret;

	ret = of_property_read_u32(dn, "reg", &host->cs);
	if (ret) {
		dev_err(dev, "can't get chip-select\n");
		return -ENXIO;
	}

	nand_set_flash_node(chip, dn);
	mtd->name = devm_kasprintf(dev, GFP_KERNEL, "qcom_nand.%d", host->cs);
	if (!mtd->name)
		return -ENOMEM;

	mtd->owner = THIS_MODULE;
	mtd->dev.parent = dev;

	chip->legacy.cmdfunc	= qcom_nandc_command;
	chip->legacy.select_chip	= qcom_nandc_select_chip;
	chip->legacy.read_byte	= qcom_nandc_read_byte;
	chip->legacy.read_buf	= qcom_nandc_read_buf;
	chip->legacy.write_buf	= qcom_nandc_write_buf;
	chip->legacy.set_features	= nand_get_set_features_notsupp;
	chip->legacy.get_features	= nand_get_set_features_notsupp;

	/*
	 * the bad block marker is readable only when we read the last codeword
	 * of a page with ECC disabled. currently, the nand_base and nand_bbt
	 * helpers don't allow us to read BB from a nand chip with ECC
	 * disabled (MTD_OPS_PLACE_OOB is set by default). use the block_bad
	 * and block_markbad helpers until we permanently switch to using
	 * MTD_OPS_RAW for all drivers (with the help of badblockbits)
	 */
	chip->legacy.block_bad		= qcom_nandc_block_bad;
	chip->legacy.block_markbad	= qcom_nandc_block_markbad;

	chip->controller = &nandc->controller;
	chip->options |= NAND_NO_SUBPAGE_WRITE | NAND_USE_BOUNCE_BUFFER |
			 NAND_SKIP_BBTSCAN;

	/* set up initial status value */
	host->status = NAND_STATUS_READY | NAND_STATUS_WP;

	if (nandc->props->is_serial_nand) {
		qspi_nand_init(nandc);
		if (nandc->boot_layout)
			ret = nand_scan_with_ids(chip, 1, qspinand_flash_ids_2k);
		else
			ret = nand_scan(chip, 1);
	} else {
		ret = nand_scan(chip, 1);
	}
	if (ret) {
		dev_err(nandc->dev, "nand scan returned error\n");
		return ret;
	}

	if (nandc->props->qpic_v2 && nandc->props->page_scope) {
		nandc->sts_buf_size = (mtd->writesize / NANDC_STEP_SIZE) *
							MAX_STATUS_REG;
		nandc->status_buf = devm_kzalloc(nandc->dev, nandc->sts_buf_size,
				GFP_KERNEL);
		if (!nandc->status_buf)
			return -ENOMEM;
	}

	/* QSPI serial training is required if io_macro clk frequency
	 * is more than 50MHz. This is due to different PNR and PCB delays,
	 * serial read data can come with different delays to QPIC. So
	 * Rx clock should be adjusted according to delays so that Rx Data
	 * can be captured correctly.
	 */
	if (nandc->props->is_serial_nand) {
		ret = qspi_nand_device_config(nandc, host, mtd);
		if (ret)
			dev_err(nandc->dev, "qspi_nand device config failed\n");
		if (nandc->props->is_serial_training) {
			if (nandc->boot_layout) {
				dev_info(nandc->dev,
				"Skip serial training in boot layout\n");
			} else {
				ret = qspi_execute_training(nandc, host, mtd);
				if (ret)
					dev_err(nandc->dev,
					"failed to enable serial training\n");
			}
		}
	}

	ret = mtd_device_register(mtd, NULL, 0);
	if (ret)
		nand_cleanup(chip);

	return ret;
}

static int qcom_probe_nand_devices(struct qcom_nand_controller *nandc)
{
	struct device *dev = nandc->dev;
	struct device_node *dn = dev->of_node, *child;
	struct qcom_nand_host *host;
	int ret = -ENODEV;

	for_each_available_child_of_node(dn, child) {
		host = devm_kzalloc(dev, sizeof(*host), GFP_KERNEL);
		if (!host) {
			of_node_put(child);
			return -ENOMEM;
		}

		ret = qcom_nand_host_init_and_register(nandc, host, child);
		if (ret) {
			devm_kfree(dev, host);
			continue;
		}

		list_add_tail(&host->node, &nandc->host_list);
	}

	return ret;
}

/* parse custom DT properties here */
static int qcom_nandc_parse_dt(struct platform_device *pdev)
{
	struct qcom_nand_controller *nandc = platform_get_drvdata(pdev);
	struct device_node *np = nandc->dev->of_node;
	int ret;

	if (!nandc->props->is_bam) {
		ret = of_property_read_u32(np, "qcom,cmd-crci",
					   &nandc->cmd_crci);
		if (ret) {
			dev_err(nandc->dev, "command CRCI unspecified\n");
			return ret;
		}

		ret = of_property_read_u32(np, "qcom,data-crci",
					   &nandc->data_crci);
		if (ret) {
			dev_err(nandc->dev, "data CRCI unspecified\n");
			return ret;
		}
	}

	return 0;
}

static ssize_t boot_layout_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct qcom_nand_controller *nandc = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u\n", nandc->boot_layout);
}

static ssize_t boot_layout_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t n)
{
	struct qcom_nand_controller *nandc = dev_get_drvdata(dev);
	struct qcom_nand_host *host, *tmp;
	int ret;

	ret = kstrtobool(buf, &nandc->boot_layout);
	if (ret) {
		dev_err(dev, "invalid boot_layout\n");
		return ret;
	}

	list_for_each_entry_safe_reverse(host, tmp, &nandc->host_list, node) {
		struct nand_chip *chip = &host->chip;
		struct mtd_info *mtd = nand_to_mtd(chip);

		ret = mtd_device_unregister(mtd);
		if (ret) {
			dev_err(dev, "device unregister failed\n");
			return ret;
		}
		memset(mtd, 0, sizeof(struct mtd_info));
		list_del(&host->node);
		devm_kfree(nandc->dev, host);
	}

	if (nandc->props->qpic_v2 && nandc->props->page_scope) {
		devm_kfree(nandc->dev, nandc->status_buf);
	}

	ret = qcom_probe_nand_devices(nandc);
	if (ret) {
		dev_err(dev, "nand device probe failed\n");
		return ret;
	}

	return n;
}

static const DEVICE_ATTR(boot_layout, 0644, boot_layout_show,
			boot_layout_store);

static int qcom_nandc_probe(struct platform_device *pdev)
{
	struct qcom_nand_controller *nandc;
	struct qcom_nand_host *host;
	const void *dev_data;
	struct device *dev = &pdev->dev;
	struct resource *res;
	int ret;

	nandc = devm_kzalloc(&pdev->dev, sizeof(*nandc), GFP_KERNEL);
	if (!nandc)
		return -ENOMEM;

	platform_set_drvdata(pdev, nandc);
	nandc->dev = dev;

	dev_data = of_device_get_match_data(dev);
	if (!dev_data) {
		dev_err(&pdev->dev, "failed to get device data\n");
		return -ENODEV;
	}

	nandc->props = dev_data;

	nandc->core_clk = devm_clk_get(dev, "core");
	if (IS_ERR(nandc->core_clk))
		return PTR_ERR(nandc->core_clk);

	nandc->aon_clk = devm_clk_get(dev, "aon");
	if (IS_ERR(nandc->aon_clk))
		return PTR_ERR(nandc->aon_clk);

	if (nandc->props->is_serial_nand) {
		nandc->iomacro_clk = devm_clk_get(dev, "io_macro");
		if (IS_ERR(nandc->iomacro_clk))
			return PTR_ERR(nandc->iomacro_clk);

		ret =  clk_set_rate(nandc->iomacro_clk, 200000000);
		if (ret)
			return ret;
	}

	ret = qcom_nandc_parse_dt(pdev);
	if (ret)
		return ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	nandc->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(nandc->base))
		return PTR_ERR(nandc->base);

	nandc->base_phys = res->start;
	nandc->base_dma = dma_map_resource(dev, res->start,
					   resource_size(res),
					   DMA_BIDIRECTIONAL, 0);
	if (!nandc->base_dma)
		return -ENXIO;

	ret = clk_prepare_enable(nandc->core_clk);
	if (ret)
		goto err_core_clk;

	ret = clk_prepare_enable(nandc->aon_clk);
	if (ret)
		goto err_aon_clk;

	if (nandc->props->is_serial_nand) {
		ret = clk_prepare_enable(nandc->iomacro_clk);
		if (ret)
			goto err_setup;
	}

	ret = qcom_nandc_alloc(nandc);
	if (ret)
		goto err_nandc_alloc;

	ret = qcom_nandc_setup(nandc);
	if (ret)
		goto err_setup;

	ret = qcom_probe_nand_devices(nandc);
	if (ret)
		goto err_setup;

	if (nandc->props->is_serial_nand && nandc->props->switch_layout) {
		list_for_each_entry(host, &nandc->host_list, node) {
			struct nand_chip *chip = &host->chip;
			struct mtd_info *mtd = nand_to_mtd(chip);

			if (mtd->writesize == SZ_4K) {
				ret = sysfs_create_file(&pdev->dev.kobj,
							&dev_attr_boot_layout.attr);
				if (ret)
					goto err_setup;
			}
		}
	}

	return 0;

err_setup:
	qcom_nandc_unalloc(nandc);
err_nandc_alloc:
	clk_disable_unprepare(nandc->aon_clk);
err_aon_clk:
	clk_disable_unprepare(nandc->core_clk);
err_core_clk:
	dma_unmap_resource(dev, nandc->base_dma, resource_size(res),
			   DMA_BIDIRECTIONAL, 0);
	return ret;
}

static int qcom_nandc_remove(struct platform_device *pdev)
{
	struct qcom_nand_controller *nandc = platform_get_drvdata(pdev);
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	struct qcom_nand_host *host;

	if (!res)
		return -EINVAL;

	list_for_each_entry(host, &nandc->host_list, node)
		nand_release(&host->chip);

	if (nandc->props->switch_layout)
		sysfs_remove_file(&pdev->dev.kobj, &dev_attr_boot_layout.attr);

	qcom_nandc_unalloc(nandc);

	clk_disable_unprepare(nandc->aon_clk);
	clk_disable_unprepare(nandc->core_clk);

	dma_unmap_resource(&pdev->dev, nandc->base_dma, resource_size(res),
			   DMA_BIDIRECTIONAL, 0);

	return 0;
}

static const struct qcom_nandc_props ipq806x_nandc_props = {
	.ecc_modes = (ECC_RS_4BIT | ECC_BCH_8BIT),
	.is_bam = false,
	.dev_cmd_reg_start = 0x0,
	.is_serial_nand = false,
};

static const struct qcom_nandc_props ipq4019_nandc_props = {
	.ecc_modes = (ECC_BCH_4BIT | ECC_BCH_8BIT),
	.is_bam = true,
	.is_qpic = true,
	.dev_cmd_reg_start = 0x0,
	.is_serial_nand = false,
};

static const struct qcom_nandc_props ipq8074_nandc_props = {
	.ecc_modes = (ECC_BCH_4BIT | ECC_BCH_8BIT),
	.is_bam = true,
	.is_qpic = true,
	.dev_cmd_reg_start = 0x7000,
	.is_serial_nand = false,
};

static const struct qcom_nandc_props ipq5018_nandc_props = {
	.ecc_modes = (ECC_BCH_4BIT | ECC_BCH_8BIT),
	.is_bam = true,
	.is_serial_nand = true,
	.qpic_v2 = true,
	.is_serial_training = true,
	.quad_mode = true,
	.page_scope = true,
	.switch_layout = true,
	.dev_cmd_reg_start = 0x7000,
};

static const struct qcom_nandc_props ipq5332_nandc_props = {
	.ecc_modes = (ECC_BCH_4BIT | ECC_BCH_8BIT),
	.is_bam = true,
	.is_serial_nand = true,
	.qpic_v2 = true,
	.is_serial_training = true,
	.quad_mode = true,
	.page_scope = true,
	.switch_layout = false,
	.dev_cmd_reg_start = 0x7000,
};

static const struct qcom_nandc_props ipq9574_nandc_props = {
	.ecc_modes = (ECC_BCH_4BIT | ECC_BCH_8BIT),
	.is_bam = true,
	.is_serial_nand = true,
	.qpic_v2 = true,
	.is_serial_training = true,
	.quad_mode = true,
	.page_scope = true,
	.switch_layout = true,
	.dev_cmd_reg_start = 0x7000,
};

/*
 * data will hold a struct pointer containing more differences once we support
 * more controller variants
 */
static const struct of_device_id qcom_nandc_of_match[] = {
	{
		.compatible = "qcom,ipq806x-nand",
		.data = &ipq806x_nandc_props,
	},
	{
		.compatible = "qcom,ipq4019-nand",
		.data = &ipq4019_nandc_props,
	},
	{
		.compatible = "qcom,ipq8074-nand",
		.data = &ipq8074_nandc_props,
	},
	{
		.compatible = "qcom,ebi2-nandc-bam-v1.5.0",
		.data = &ipq8074_nandc_props,
	},
	{
		.compatible = "qcom,ebi2-nandc-bam-v2.1.1",
		.data = &ipq5018_nandc_props,
	},
	{
		.compatible = "qcom,ipq5332-nand",
		.data = &ipq5332_nandc_props,
	},
	{
		.compatible = "qcom,ipq9574-nand",
		.data = &ipq9574_nandc_props,
	},
	{}
};
MODULE_DEVICE_TABLE(of, qcom_nandc_of_match);

static struct platform_driver qcom_nandc_driver = {
	.driver = {
		.name = "qcom-nandc",
		.of_match_table = qcom_nandc_of_match,
	},
	.probe   = qcom_nandc_probe,
	.remove  = qcom_nandc_remove,
};
module_platform_driver(qcom_nandc_driver);

MODULE_AUTHOR("Archit Taneja <architt@codeaurora.org>");
MODULE_DESCRIPTION("Qualcomm NAND Controller driver");
MODULE_LICENSE("GPL v2");
