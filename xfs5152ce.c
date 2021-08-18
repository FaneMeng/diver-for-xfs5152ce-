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

#define XFS5152CE_MINOR_NUMBER 0 //次设备号
#define XFS5152CE_CNT 1
#define XFS5152CE_NAME "xfs5152ce"
struct xfs5152ce_dev{
	int major;
	int minor;
	dev_t devid;	  /*描述设备号的数据类型*/
	struct cdev cdev; /*用cdev结构体来描述一个字符设备*/
	struct class *class;
	struct device *device;
	void *priavte_data;
};

static struct xfs5152ce_dev xfs5152cedev;  //定义设备描述结构体

static int xfs5152ce_open (struct inode *inode, struct file *filp)
{
	return 0;

}

ssize_t xfs5152ce_read (struct file *flip, char __user *buf, size_t cnt, loff_t *off)
{
	return 0;

}

ssize_t xfs5152ce_write (struct file *flip, const char __user *buf, size_t cnt, loff_t *off)
{
	return 0;
}

static int xfs5152ce_release (struct inode *inode, struct file *filp)
{
	return 0;
}

static const struct file_operations xfs5152ce_fops={
	.owner = THIS_MODULE,
	.open = xfs5152ce_open,
	.read = xfs5152ce_read,
	.write = xfs5152ce_write,
	.release = xfs5152ce_release
};


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

	/*设备号获取*/
	xfs5152cedev.major = 0;
	if(xfs5152cedev.major) //由系统分配主设备号
	{
		xfs5152cedev.devid = MKDEV(xfs5152cedev.major,XFS5152CE_MINOR_NUMBER);//设备号合成
		ret = register_chrdev_region(xfs5152cedev.devid, XFS5152CE_CNT, XFS5152CE_NAME);  //register a range of device numbers
	}
	else  //没有给定主设备号
	{
		ret = alloc_chrdev_region(&xfs5152cedev.devid, XFS5152CE_MINOR_NUMBER, XFS5152CE_CNT,XFS5152CE_NAME);
		xfs5152cedev.major = MAJOR(xfs5152cedev.devid);
		xfs5152cedev.minor = MINOR(xfs5152cedev.devid);
	}
	if(ret < 0)
	{
		printk("xfs5152ce chrdev_refion error!\r\n");
		goto FAIL_DEVID;
	}
	printk("xfs5152ce major=%d, minor = %d\r\n",xfs5152cedev.major,xfs5152cedev.minor);

	/*注册字符设备*/
	xfs5152cedev.cdev.owner = THIS_MODULE;
	cdev_init(&xfs5152cedev.cdev,&xfs5152ce_fops);
	ret = cdev_add(&xfs5152cedev.cdev,xfs5152cedev.devid,XFS5152CE_CNT);
	if(ret < 0)
	{
		printk("xfs5152ce cdev add error!\r\n");
		goto FAIL_CDEV;
	}

	/*自动创建设备节点*/
	xfs5152cedev.class = class_create(THIS_MODULE,XFS5152CE_NAME);
	if(IS_ERR(xfs5152cedev.class))
	{
		ret = PTR_ERR(xfs5152cedev.class);
		printk("xfs5152ce class create error!\r\n");
		goto FAIL_CLASS;
	}
	xfs5152cedev.device = device_create(xfs5152cedev.class,NULL,xfs5152cedev.devid,NULL,XFS5152CE_NAME);
	if(IS_ERR(xfs5152cedev.device))
	{
		ret = PTR_ERR(xfs5152cedev.device);
		printk("xfs5152ce device node create error!\r\n");
		goto FAIL_DEVICE;
	}
	/*私有数据，spi结构体*/
	xfs5152cedev.priavte_data = spi;
	return 0;	
	
FAIL_DEVICE:
	class_destroy(xfs5152cedev.class);
FAIL_CLASS:
	cdev_del(&xfs5152cedev.cdev);
FAIL_CDEV:
	unregister_chrdev_region(xfs5152cedev.devid,XFS5152CE_CNT);
FAIL_DEVID:
	return ret;

}

/*
 * @description     : spi驱动的remove函数，移除spi驱动的时候此函数会执行
 * @param - client 	: spi设备
 * @return          : 0，成功;其他负值,失败
 */
static int xfs5152ce_remove(struct spi_device *spi)
{
	xfs5152cedev.priavte_data = NULL;	
	/*注销设备号*/
	unregister_chrdev_region(xfs5152cedev.devid,XFS5152CE_CNT);
	/*删除字符设备*/
	cdev_del(&xfs5152cedev.cdev);
	/*摧毁设备节点*/
	device_destroy(xfs5152cedev.class,xfs5152cedev.devid);
	/*摧毁类*/
	class_destroy(xfs5152cedev.class);
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
static int  xfs5152ce_init(void)
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
static void  xfs5152ce_exit(void)
{
	spi_unregister_driver(&xfs5152ce_driver);	
}

module_init(xfs5152ce_init);
module_exit(xfs5152ce_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("mengfy1995@foxmail.com");



