#define pr_fmt(fmt) "n2:%s:%s(): " fmt, KBUILD_MODNAME, __func__
#define dev_fmt(fmt) pr_fmt(fmt)

#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("n2 h9");
MODULE_DESCRIPTION("I2C driver for bmp280 pressure temperature sensor");

// this is for my IDE, it keeps showing me KBUILD_MODNAME is not defined
#ifndef KBUILD_MODNAME
#define KBUILD_MODNAME "n2_i2c_bmp280"
#endif

#define COMPENSATION_PARAM_REG_ADDR 0x88
#define COMPENSATION_PARAM_DATA_SIZE_BYTES 24

#define CONTROL_MEASUREMENT_REG_ADDR 0xF4
#define CONFIGURATION_REG_ADDR 0xF5

#define DATA_REG_ADDR 0xF7
#define DATA_SIZE_BYTES 8

typedef s32 BMP280_S32_t;
typedef u32 BMP280_U32_t;
typedef s64 BMP280_S64_t;

static const struct of_device_id bmp280_dt_ids[] = { {
							     .compatible =
								     "n2bmp280",
						     },
						     {} };
MODULE_DEVICE_TABLE(of, bmp280_dt_ids);

static const struct i2c_device_id i2c_ids[] = { {
							.name = "n2bmp280",
						},
						{} };
MODULE_DEVICE_TABLE(i2c, i2c_ids);

/* This structure represents single device */
struct bmp280_dev {
	struct i2c_client *client;
	struct miscdevice bmp280_miscdevice;
	char name[12]; /* n2bmp280_XX */
};

// function to obtain main dev pointer from file struct
static struct bmp280_dev *get_main_dev_from_file(struct file *file)
{
	struct miscdevice *misc_dev;
	struct bmp280_dev *main_dev;

	if (file == NULL) {
		pr_err("file is NULL:\n");
		return NULL;
	}

	// Retrieve miscdevice structure from file->private_data
	misc_dev = file->private_data;
	if (misc_dev == NULL) {
		pr_err("Failed to retrieve misc device\n");
		return NULL;
	}
	pr_debug("Successfully got misc device: %s from file\n",
		 misc_dev->name);

	// Retrieve master device structure from misc device
	main_dev = dev_get_drvdata(misc_dev->this_device);
	if (main_dev == NULL) {
		pr_err("Failed to retrieve main device from misc device\n");
		return NULL;
	}
	pr_debug("Successfully got main device: %s from misc device\n",
		 main_dev->name);

	return main_dev;
}

/* User is reading data from /dev/n2bmp280XX */
static ssize_t bmp280_read_file(struct file *file, char __user *userbuf,
				size_t count, loff_t *ppos)
{
	struct bmp280_dev *main_dev;
	// read count from i2c device
	int rc;
	// compensation_data to read from i2c device
	u8 compensation_data[COMPENSATION_PARAM_DATA_SIZE_BYTES];
	u8 data[DATA_SIZE_BYTES];

	pr_debug("start read file");

	main_dev = get_main_dev_from_file(file);

	rc = i2c_smbus_read_i2c_block_data(main_dev->client,
					   COMPENSATION_PARAM_REG_ADDR,
					   sizeof(compensation_data),
					   compensation_data);
	if (rc != sizeof(compensation_data)) {
		dev_err(&main_dev->client->dev,
			"Failed to read compensation_data, rc=%d\n", rc);
		return -EIO;
	}
	dev_dbg(&main_dev->client->dev,
		"Successfully read compensation_data from device, rc=%d\n", rc);

	// Convert the data
	// temp coefficents
	int dig_T1 = compensation_data[1] * 256 + compensation_data[0];
	int dig_T2 = compensation_data[3] * 256 + compensation_data[2];
	if (dig_T2 > 32767) {
		dig_T2 -= 65536;
	}
	int dig_T3 = compensation_data[5] * 256 + compensation_data[4];
	if (dig_T3 > 32767) {
		dig_T3 -= 65536;
	}
	// pressure coefficents
	int dig_P1 = compensation_data[7] * 256 + compensation_data[6];
	int dig_P2 = compensation_data[9] * 256 + compensation_data[8];
	if (dig_P2 > 32767) {
		dig_P2 -= 65536;
	}
	int dig_P3 = compensation_data[11] * 256 + compensation_data[10];
	if (dig_P3 > 32767) {
		dig_P3 -= 65536;
	}
	int dig_P4 = compensation_data[13] * 256 + compensation_data[12];
	if (dig_P4 > 32767) {
		dig_P4 -= 65536;
	}
	int dig_P5 = compensation_data[15] * 256 + compensation_data[14];
	if (dig_P5 > 32767) {
		dig_P5 -= 65536;
	}
	int dig_P6 = compensation_data[17] * 256 + compensation_data[16];
	if (dig_P6 > 32767) {
		dig_P6 -= 65536;
	}
	int dig_P7 = compensation_data[19] * 256 + compensation_data[18];
	if (dig_P7 > 32767) {
		dig_P7 -= 65536;
	}
	int dig_P8 = compensation_data[21] * 256 + compensation_data[20];
	if (dig_P8 > 32767) {
		dig_P8 -= 65536;
	}
	int dig_P9 = compensation_data[23] * 256 + compensation_data[22];
	if (dig_P9 > 32767) {
		dig_P9 -= 65536;
	}

	// Read data back from DATA_REG_ADDR, DATA_SIZE_BYTES bytes
	// Pressure MSB, Pressure LSB, Pressure xLSB, Temperature MSB, Temperature LSB
	// Temperature xLSB, Humidity MSB, Humidity LSB
	rc = i2c_smbus_read_i2c_block_data(main_dev->client, DATA_REG_ADDR,
					   sizeof(data), data);
	if (rc != sizeof(data)) {
		dev_err(&main_dev->client->dev,
			"Failed to read main data, rc=%d\n", rc);
		return -EIO;
	}
	dev_dbg(&main_dev->client->dev, "Successfully read main data, rc=%d\n",
		rc);

	// t_fine is used in both temperature and pressure calculations
	BMP280_S32_t t_fine;

	// Temperature calculation
	//
	// cTemp will contain temperature in DegC, resolution 0.01 DegC.
	// Output value of 5123 equals to 51.23 DegC.
	BMP280_S32_t cTemp;
	{
		// Convert temperature data to 19-bits
		BMP280_S32_t adc_t = (((BMP280_S32_t)data[3] * 65536) +
				      ((BMP280_S32_t)data[4] * 256) +
				      (BMP280_S32_t)(data[5] & 0xF0)) >>
				     4;

		// Temperature offset calculations
		BMP280_S32_t var1, var2;

		var1 = ((((adc_t >> 3) - ((BMP280_S32_t)dig_T1 << 1))) *
			((BMP280_S32_t)dig_T2)) >>
		       11;
		var2 = (((((adc_t >> 4) - ((BMP280_S32_t)dig_T1)) *
			  ((adc_t >> 4) - ((BMP280_S32_t)dig_T1))) >>
			 12) *
			((BMP280_S32_t)dig_T3)) >>
		       14;
		t_fine = var1 + var2;
		cTemp = (t_fine * 5 + 128) >> 8;
	}

	// Pressure calculation
	//
	// pressure contains pressure in Pa as unsigned 32 bit integer in Q2.8 format
	// (24 integer bits and 8 fractional bits).
	// value of 24674867 represents 24674867/256 = 96386.2 Pa = 963.862 hPa
	BMP280_U32_t pressure;
	{
		// Convert pressure data to 19-bits
		BMP280_S32_t adc_p = (((BMP280_S64_t)data[0] * 65536) +
				      ((BMP280_S64_t)data[1] * 256) +
				      (BMP280_S64_t)(data[2] & 0xF0)) >>
				     4;

		// Pressure offset calculations
		BMP280_S64_t var1, var2, p;

		var1 = ((BMP280_S64_t)t_fine) - 128000;
		var2 = var1 * var1 * (BMP280_S64_t)dig_P6;
		var2 = var2 + ((var1 * (BMP280_S64_t)dig_P5) << 17);
		var2 = var2 + (((BMP280_S64_t)dig_P4) << 35);
		var1 = ((var1 * var1 * (BMP280_S64_t)dig_P3) >> 8) +
		       ((var1 * (BMP280_S64_t)dig_P2) << 12);
		var1 = (((((BMP280_S64_t)1) << 47) + var1)) *
			       ((BMP280_S64_t)dig_P1) >>
		       33;
		if (var1 == 0) {
			pressure =
				0; // avoid exception caused by devision by zero
		} else {
			p = 1048576 - adc_p;
			p = (((p << 31) - var2) * 3125) / var1;
			var1 = (((BMP280_S64_t)dig_P9) * (p >> 13) *
				(p >> 13)) >>
			       25;
			var2 = (((BMP280_S64_t)dig_P8) * p) >> 19;
			p = ((p + var1 + var2) >> 8) +
			    (((BMP280_S64_t)dig_P7) << 4);
			pressure = (BMP280_U32_t)p;
		}
	}

	// Output data to logs
	dev_info(&main_dev->client->dev, "Temperature: %d/100 C \n", cTemp);
	dev_info(&main_dev->client->dev, "Pressure: %d/256 Pa \n", pressure);

	char res[16];
	sprintf(res, "%d\n%d\n", cTemp, pressure);
	size_t res_len = strlen(res);

	int bytes_not_copied = copy_to_user(userbuf, res, res_len);
	if (bytes_not_copied) {
		dev_err(&main_dev->client->dev,
			"copy to user failed, not copied bytes = %d\n",
			bytes_not_copied);
		return -EFAULT;
	}

	return res_len;
}

/* User is writting data to /dev/n2bmp280XX */
static ssize_t bmp280_write_file(struct file *file, const char __user *userbuf,
				 size_t count, loff_t *ppos)
{
	// configure sensor on any write with default options:
	// - Normal mode
	// - Stand_by time = 1000 ms
	// - Pressure and Temperature Oversampling rate = 1
	// --
	// TODO: maybe implement opportunity to configure sensor
	// --

	struct bmp280_dev *main_dev;
	// read count from i2c device
	int rc;

	pr_debug("start write file");

	main_dev = get_main_dev_from_file(file);

	//		0x27	Pressure and Temperature Oversampling rate = 1
	//					Normal mode
	rc = i2c_smbus_write_byte_data(main_dev->client,
				       CONTROL_MEASUREMENT_REG_ADDR, 0x27);
	if (rc < 0) {
		dev_err(&main_dev->client->dev,
			"Failed to write byte data to CONTROL_MEASUREMENT_REG_ADDR, "
			"rc=%d\n",
			rc);
		return -EIO;
	}
	dev_dbg(&main_dev->client->dev,
		"Successfully wrote byte data to CONTROL_MEASUREMENT_REG_ADDR, rc=%d\n",
		rc);

	//		0xA0(00)	Stand_by time = 1000 ms
	rc = i2c_smbus_write_byte_data(main_dev->client, CONFIGURATION_REG_ADDR,
				       0xA0);
	if (rc < 0) {
		dev_err(&main_dev->client->dev,
			"Failed to write byte data to CONFIGURATION_REG_ADDR, rc=%d\n",
			rc);
		return -EIO;
	}
	dev_dbg(&main_dev->client->dev,
		"Successfully wrote byte data to CONFIGURATION_REG_ADDR, rc=%d\n",
		rc);

	return count;
}

static const struct file_operations bmp280_fops = {
	.owner = THIS_MODULE,
	.read = bmp280_read_file,
	.write = bmp280_write_file,
};

static int bmp280_probe(struct i2c_client *client)
{
	static int counter = 0;
	int return_value;
	struct bmp280_dev *dev;

	pr_debug("start probe");

	/* Allocate new structure representing device */
	dev = devm_kzalloc(&client->dev, sizeof(struct bmp280_dev), GFP_KERNEL);

	/* Store pointer to the device-structure in bus device context */
	i2c_set_clientdata(client, dev);

	/* Store pointer to I2C device/client */
	dev->client = client;

	/* Initialize the misc device, incremented after each probe call */
	sprintf(dev->name, "n2bmp280_%02d", counter++);

	dev->bmp280_miscdevice.name = dev->name;
	dev->bmp280_miscdevice.minor = MISC_DYNAMIC_MINOR;
	dev->bmp280_miscdevice.fops = &bmp280_fops;

	/* Register misc device */
	return_value = misc_register(&dev->bmp280_miscdevice);
	if (return_value != 0) {
		pr_err("Failed to register misc device; error code = %d\n",
		       return_value);
	}

	if (dev->bmp280_miscdevice.this_device == NULL) {
		pr_err("dev->bmp280_miscdevice.this_device is NULL");
	} else {
		// save relation to device data structure
		dev_set_drvdata(dev->bmp280_miscdevice.this_device, dev);
	}

	return return_value;
}

static void bmp280_remove(struct i2c_client *client)
{
	pr_debug("start remove");
	struct bmp280_dev *dev;

	/* Get device structure from bus device context */
	dev = i2c_get_clientdata(client);

	/* Deregister misc device */
	misc_deregister(&dev->bmp280_miscdevice);
}

static struct i2c_driver bmp280_driver = {
    .driver =
        {
            .name = "n2bmp280",
            .owner = THIS_MODULE,
            .of_match_table = bmp280_dt_ids,
        },
    .probe = bmp280_probe,
    .remove = bmp280_remove,
    .id_table = i2c_ids,
};

module_i2c_driver(bmp280_driver);