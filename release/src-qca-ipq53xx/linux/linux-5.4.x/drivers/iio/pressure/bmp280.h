/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/bitops.h>
#include <linux/device.h>
#include <linux/regmap.h>

/* BMP280 specific registers */
#define BMP280_REG_HUMIDITY_LSB		0xFE
#define BMP280_REG_HUMIDITY_MSB		0xFD
#define BMP280_REG_TEMP_XLSB		0xFC
#define BMP280_REG_TEMP_LSB		0xFB
#define BMP280_REG_TEMP_MSB		0xFA
#define BMP280_REG_PRESS_XLSB		0xF9
#define BMP280_REG_PRESS_LSB		0xF8
#define BMP280_REG_PRESS_MSB		0xF7

#define BMP280_REG_CONFIG		0xF5
#define BMP280_REG_CTRL_MEAS		0xF4
#define BMP280_REG_STATUS		0xF3
#define BMP280_REG_CTRL_HUMIDITY	0xF2

/* Due to non linear mapping, and data sizes we can't do a bulk read */
#define BMP280_REG_COMP_H1		0xA1
#define BMP280_REG_COMP_H2		0xE1
#define BMP280_REG_COMP_H3		0xE3
#define BMP280_REG_COMP_H4		0xE4
#define BMP280_REG_COMP_H5		0xE5
#define BMP280_REG_COMP_H6		0xE7

#define BMP280_REG_COMP_TEMP_START	0x88
#define BMP280_COMP_TEMP_REG_COUNT	6

#define BMP280_REG_COMP_PRESS_START	0x8E
#define BMP280_COMP_PRESS_REG_COUNT	18

#define BMP280_FILTER_MASK		(BIT(4) | BIT(3) | BIT(2))
#define BMP280_FILTER_OFF		0
#define BMP280_FILTER_2X		BIT(2)
#define BMP280_FILTER_4X		BIT(3)
#define BMP280_FILTER_8X		(BIT(3) | BIT(2))
#define BMP280_FILTER_16X		BIT(4)

#define BMP280_OSRS_HUMIDITY_MASK	(BIT(2) | BIT(1) | BIT(0))
#define BMP280_OSRS_HUMIDITIY_X(osrs_h)	((osrs_h) << 0)
#define BMP280_OSRS_HUMIDITY_SKIP	0
#define BMP280_OSRS_HUMIDITY_1X		BMP280_OSRS_HUMIDITIY_X(1)
#define BMP280_OSRS_HUMIDITY_2X		BMP280_OSRS_HUMIDITIY_X(2)
#define BMP280_OSRS_HUMIDITY_4X		BMP280_OSRS_HUMIDITIY_X(3)
#define BMP280_OSRS_HUMIDITY_8X		BMP280_OSRS_HUMIDITIY_X(4)
#define BMP280_OSRS_HUMIDITY_16X	BMP280_OSRS_HUMIDITIY_X(5)

#define BMP280_OSRS_TEMP_MASK		(BIT(7) | BIT(6) | BIT(5))
#define BMP280_OSRS_TEMP_SKIP		0
#define BMP280_OSRS_TEMP_X(osrs_t)	((osrs_t) << 5)
#define BMP280_OSRS_TEMP_1X		BMP280_OSRS_TEMP_X(1)
#define BMP280_OSRS_TEMP_2X		BMP280_OSRS_TEMP_X(2)
#define BMP280_OSRS_TEMP_4X		BMP280_OSRS_TEMP_X(3)
#define BMP280_OSRS_TEMP_8X		BMP280_OSRS_TEMP_X(4)
#define BMP280_OSRS_TEMP_16X		BMP280_OSRS_TEMP_X(5)

#define BMP280_OSRS_PRESS_MASK		(BIT(4) | BIT(3) | BIT(2))
#define BMP280_OSRS_PRESS_SKIP		0
#define BMP280_OSRS_PRESS_X(osrs_p)	((osrs_p) << 2)
#define BMP280_OSRS_PRESS_1X		BMP280_OSRS_PRESS_X(1)
#define BMP280_OSRS_PRESS_2X		BMP280_OSRS_PRESS_X(2)
#define BMP280_OSRS_PRESS_4X		BMP280_OSRS_PRESS_X(3)
#define BMP280_OSRS_PRESS_8X		BMP280_OSRS_PRESS_X(4)
#define BMP280_OSRS_PRESS_16X		BMP280_OSRS_PRESS_X(5)

#define BMP280_MODE_MASK		(BIT(1) | BIT(0))
#define BMP280_MODE_SLEEP		0
#define BMP280_MODE_FORCED		BIT(0)
#define BMP280_MODE_NORMAL		(BIT(1) | BIT(0))

/* BMP180 specific registers */
#define BMP180_REG_OUT_XLSB		0xF8
#define BMP180_REG_OUT_LSB		0xF7
#define BMP180_REG_OUT_MSB		0xF6

#define BMP180_REG_CALIB_START		0xAA
#define BMP180_REG_CALIB_COUNT		22

#define BMP180_MEAS_SCO			BIT(5)
#define BMP180_MEAS_TEMP		(0x0E | BMP180_MEAS_SCO)
#define BMP180_MEAS_PRESS_X(oss)	((oss) << 6 | 0x14 | BMP180_MEAS_SCO)
#define BMP180_MEAS_PRESS_1X		BMP180_MEAS_PRESS_X(0)
#define BMP180_MEAS_PRESS_2X		BMP180_MEAS_PRESS_X(1)
#define BMP180_MEAS_PRESS_4X		BMP180_MEAS_PRESS_X(2)
#define BMP180_MEAS_PRESS_8X		BMP180_MEAS_PRESS_X(3)

/* BMP390 specific registers */
#define BMP390_REG_TEMP_XLSB		0x07
#define BMP390_REG_TEMP_LSB		0x08
#define BMP390_REG_TEMP_MSB		0x09
#define BMP390_REG_PRESS_XLSB		0x04
#define BMP390_REG_PRESS_LSB		0x05
#define BMP390_REG_PRESS_MSB		0x06
#define BMP390_REG_OSR			0x1C
#define BMP390_REG_ODR			0x1D
#define BMP390_REG_PWR_CTRL		0x1B
#define BMP390_REG_CONFIG		0x1F
#define BMP390_REG_COMP_TEMP_START	0x31
#define BMP390_TEMP_CALIB_DATA_LEN	5
#define BMP390_REG_COMP_PRESS_START	0x36
#define BMP390_PRESS_CALIB_DATA_LEN	16
#define BMP390_REG_STATUS		0x03
#define BMP390_REG_CMD			0x7E
#define BMP390_REG_ERR			0x02
#define BMP390_REG_ID			0x00
#define BMP390_REG_REV			0x01

/* Oversampling macros */
#define BMP390_OSR_TEMP_X(osr_t)	((osr_t) << 3)
#define BMP390_OSR_TEMP_MASK		(BIT(5) | BIT(4) | BIT(3))
#define BMP390_OSR_PRESS_X(osr_p)	((osr_p) << 0)
#define BMP390_OSR_PRESS_MASK		(BIT(2) | BIT(1) | BIT(0))
#define BMP390_PRESS_EN_MASK		BIT(0)
#define BMP390_TEMP_EN_MASK		BIT(1)
#define BMP390_PRESS_EN			BIT(0)
#define BMP390_TEMP_EN			BIT(1)
/* Power mode macros */
#define BMP390_MODE_MASK                (BIT(5) | BIT(4))
#define BMP390_MODE_NORMAL		(BIT(5) | BIT(4))
#define BMP390_MODE_SLEEP		0
#define BMP390_MODE_FORCED		BIT(5)
/* IIR Filter macros */
#define BMP390_FILTER_MASK		(BIT(3) | BIT(2) | BIT(1))
#define BMP390_FILTER_COEFF_1		BIT(1)
#define BMP390_FILTER_COEFF_3		BIT(2)
#define BMP390_FILTER_COEFF_7		(BIT(2) | BIT(1))
#define BMP390_FILTER_COEFF_15		BIT(3)
#define BMP390_FILTER_COEFF_31		(BIT(3) | BIT(1))
#define BMP390_FILTER_COEFF_63		(BIT(3) | BIT(2))
#define BMP390_FILTER_COEFF_127		(BIT(3) | BIT(2) | BIT(1))
/* Status macros */
#define BMP390_CMD_READY		BIT(4)
#define BMP390_CMD_ERR			(BIT(2) | BIT(1) | BIT(0))
#define BMP390_TEMP_DRDY		BIT(6)
#define BMP390_PRESS_DRDY		BIT(5)
#define BMP390_ODR_25Hz			0x03
/* Macro to combine two 8 bit data to form a 16 bit data */
#define BMP390_CONCAT_BYTES(msb,lsb)	(((uint16_t)msb << 8 ) | (uint16_t)lsb)

/* BMP180 and BMP280 common registers */
#define BMP280_REG_CTRL_MEAS		0xF4
#define BMP280_REG_RESET		0xE0
#define BMP280_REG_ID			0xD0

#define BMP180_CHIP_ID			0x55
#define BMP280_CHIP_ID			0x58
#define BME280_CHIP_ID			0x60
/* CHIP_ID being the same for BME280 and BMP390, considering the revision id along with the chip_id for BMP390
 * CHIP_ID: 0x6001
 */
#define BMP390_CHIP_ID_VALUE		0x60
#define BMP390_REV_ID			0x01
#define BMP390_CHIP_ID			((BMP390_CHIP_ID_VALUE << 8) | (BMP390_REV_ID))
#define BMP280_SOFT_RESET_VAL		0xB6
#define BMP390_SOFT_RESET_VAL		0xB6

/* BMP280 register skipped special values */
#define BMP280_TEMP_SKIPPED		0x80000
#define BMP280_PRESS_SKIPPED		0x80000
#define BMP280_HUMIDITY_SKIPPED		0x8000

/* Regmap configurations */
extern const struct regmap_config bmp180_regmap_config;
extern const struct regmap_config bmp280_regmap_config;
extern const struct regmap_config bmp390_regmap_config;

/* Probe called from different transports */
int bmp280_common_probe(struct device *dev,
			struct regmap *regmap,
			unsigned int chip,
			const char *name,
			int irq);
int bmp280_common_remove(struct device *dev);

/* PM ops */
extern const struct dev_pm_ops bmp280_dev_pm_ops;
