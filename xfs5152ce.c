/*
 *	xfs5152ce.c
 *
 * 2021+ Copyright (c) Fanyue Meng <mengfy1995@foxmail.com>
 * All rights reserved.
 *
 * This program is a driver for xfs5152ce of data acquisition controller 
 * by Intelligent Mine Research Institute, CCTEG.
 * 
 *
 */


#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include "xfs5152reg.h"



 /*
  * @description     : spi驱动的probe函数，当驱动与
  *                    设备匹配以后此函数就会执行
  * @param - client  : spi设备
  * @param - id      : spi设备ID
  * 
  */	
static int xfs5152ce_probe(struct spi_device *spi)
{
	int ret = 0;
	printk("xfs5152ce_probe\r\n");
	return ret;	
}

/*
 * @description     : spi驱动的remove函数，移除spi驱动的时候此函数会执行
 * @param - client 	: spi设备
 * @return          : 0，成功;其他负值,失败
 */
static int xfs5152ce_remove(struct spi_device *spi)
{
	return 0;
	
}

/* 传统匹配方式ID列表 */
static const struct spi_device_id xfs5152ce_id[] = {
	{"ccteg,xfs5152ce", 0},  
	{}
};

/* 设备树匹配列表 */
static const struct of_device_id xfs5152ce_of_match[] = {
	{ .compatible = "ccteg,xfs5152ce" },
	{ /* Sentinel */ }
};

/* SPI驱动结构体 */	
static struct spi_driver xfs5152ce_driver = {
	.probe = xfs5152ce_probe,
	.remove = xfs5152ce_remove,
	.driver = {
			.owner = THIS_MODULE,
		   	.name = "xfs5152ce",
		   	.of_match_table = xfs5152ce_of_match, 
		   },
	.id_table = xfs5152ce_id
};
		   
/*
 * @description	: 驱动入口函数
 * @param 		: 无
 * @return 		: 无
 */
static int __init xfs5152ce_init(void)
{
	int ret = 0;
	ret = spi_register_driver(&xfs5152ce_driver);
	return ret;
	
}

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static void __exit xfs5152ce_exit(void)
{
	spi_unregister_device(&xfs5152ce_driver);
	
}

module_init(xfs5152ce_init);
module_exit(xfs5152ce_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("mengfy1995@foxmail.com");



