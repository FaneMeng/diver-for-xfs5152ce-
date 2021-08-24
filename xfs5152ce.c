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
#include <linux/delay.h>
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
	void *private_data;
	int cs_gpio;          //片选信号
	struct device_node *nd;  //设备树父节点
};

#if 0
static int xfs5152ce_read_regs(struct xfs5152ce_dev *dev, u8 reg, void *buf, int len)
{
	u8 data[4] = {XFS_FRAME_HEADER,0x0,0x01,reg};
/* 	u8 frame_head = XFS_FRAME_HEADER;
	u8 high_wordlen = 0x0;
	u8 low_wordlen = 0x01; */
	int ret = 0;
	struct spi_device *spi = (struct spi_device *)dev->private_data;
	gpio_set_value(dev->cs_gpio,0);
	
	/* spi_write(spi,&frame_head,1);
	gpio_set_value(dev->cs_gpio,1);
	udelay(110);
	gpio_set_value(dev->cs_gpio,0);
	spi_write(spi,&high_wordlen,1);
	gpio_set_value(dev->cs_gpio,1);
	udelay(110);
	gpio_set_value(dev->cs_gpio,0);
	spi_write(spi,&low_wordlen,1);
	gpio_set_value(dev->cs_gpio,1);
	udelay(110);
	gpio_set_value(dev->cs_gpio,0);
	ret = spi_write_then_read(spi,&data,1,buf, 1); */
	ret = spi_write_then_read(spi,data,4,buf,1);
	gpio_set_value(dev->cs_gpio,1);
	return ret;
	
}
#endif




/*SPI读寄存器*/
static int xfs5152ce_read_regs(struct xfs5152ce_dev *dev, u8 reg, void *buf, int len)
{
	int ret = 0;
	u8 txdata[len];
	struct spi_message m;
	struct spi_transfer *t;
	struct spi_device *spi = (struct spi_device *)dev->private_data;

	gpio_set_value(dev->cs_gpio,0); //拉低片选引脚
	t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL); //申请内存，进程上下文可睡眠

	/*第一次，发送帧头*/	
	txdata[0] = XFS_FRAME_HEADER;
	t->tx_buf = txdata;
	t->len = 1;
	spi_message_init(&m);
	spi_message_add_tail(t,&m);
	ret = spi_sync(spi,&m);
	if(ret<0)
	{
		printk("spi send error!\r\n");
	}
	gpio_set_value(dev->cs_gpio,1);
	udelay(110); 

	/*第二次，发送数据区长度高字节*/
	gpio_set_value(dev->cs_gpio,0);
	txdata[0] = 0;
	t->tx_buf = txdata;
	t->len = 1;
	spi_message_init(&m);
	spi_message_add_tail(t,&m);
	ret = spi_sync(spi,&m);
	if(ret<0)
	{
		printk("spi send error!\r\n");
	}
	gpio_set_value(dev->cs_gpio,1);
	udelay(110);

	/*第三次，发送数据区长度高字节*/
	gpio_set_value(dev->cs_gpio,0);
	txdata[0] = 0x01;
	t->tx_buf = txdata;
	t->len = 1;
	spi_message_init(&m);
	spi_message_add_tail(t,&m);
	ret = spi_sync(spi,&m);
	if(ret<0)
	{
		printk("spi send error!\r\n");
	}
	gpio_set_value(dev->cs_gpio,1);
	udelay(110);

	/*第四次，发送数据*/
	gpio_set_value(dev->cs_gpio,0);
	txdata[0] = reg;
	t->tx_buf = txdata;
	t->len = len;
	spi_message_init(&m);
	spi_message_add_tail(t,&m);
	ret = spi_sync(spi,&m);
	if(ret<0)
	{
		printk("spi send error!\r\n");
	}
	gpio_set_value(dev->cs_gpio,1);
	udelay(110);

	/*第五次，读取数据*/
	gpio_set_value(dev->cs_gpio,0);
	txdata[0] = 0xff;
	t->rx_buf = buf;
	t->len = len;
	spi_message_init(&m);
	spi_message_add_tail(t,&m);
	ret = spi_sync(spi,&m);
	
	kfree(t);
	gpio_set_value(dev->cs_gpio,1);

	return ret;
}


static struct xfs5152ce_dev xfs5152cedev;  //定义设备描述结构体

static int xfs5152ce_open (struct inode *inode, struct file *filp)
{
	filp->private_data = &xfs5152cedev;
	return 0;

}

ssize_t xfs5152ce_read (struct file *flip, char __user *buf, size_t cnt, loff_t *off)
{
	u8 value = 0;
	long err = 0;
	struct xfs5152ce_dev *dev = flip->private_data;
	xfs5152ce_read_regs(dev, XFS_STATE_INQUIRY, &value, 1);
	err = copy_to_user(buf,&value,1);
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







/*SPI写寄存器*/
/*static s32 xfs5152ce_write_regs(static xfs5152ce_dev *dev, u8 reg, u8 *buf, int len)
{

}*/

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
	u8 value = 0x55;
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

	/*获取片选引脚*/
	xfs5152cedev.nd = of_get_parent(spi->dev.of_node);
	xfs5152cedev.cs_gpio = of_get_named_gpio(xfs5152cedev.nd,"cs-gpio",0);
	if(xfs5152cedev.cs_gpio < 0)
	{
		printk("can't get cs-gpio\r\n");
		goto FAIL_GPIO;
	}

	ret = gpio_request(xfs5152cedev.cs_gpio,"cs");
	if(ret < 0)
	{
		printk("cs_gpio request failed!\r\n");
	}
	ret = gpio_direction_output(xfs5152cedev.cs_gpio,1);//默认高电平

	/*初始化spidevice*/
	spi->mode = SPI_MODE_0;
	spi_setup(spi);

	/*私有数据，spi结构体*/
	xfs5152cedev.private_data = spi;

	/*测试数据读取*/
	//gpio_set_value(xfs5152cedev.cs_gpio,0);
	printk("The original value  = %#X\r\n", value);
	xfs5152ce_read_regs(&xfs5152cedev, XFS_STATE_INQUIRY, &value, 1);
	printk("The status of xfs5152ce  = %#X\r\n", value);
	//xfs5152ce_read_regs(&xfs5152cedev, XFS_AWAKE, &value, 1);	
	//printk("The status of xfs5152ce  = %#X\r\n", value);
	printk("what!\r\n");
	return 0;

FAIL_GPIO:	
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
	xfs5152cedev.private_data = NULL;	
	/*注销设备号*/
	unregister_chrdev_region(xfs5152cedev.devid,XFS5152CE_CNT);
	/*删除字符设备*/
	cdev_del(&xfs5152cedev.cdev);
	/*摧毁设备节点*/
	device_destroy(xfs5152cedev.class,xfs5152cedev.devid);
	/*摧毁类*/
	class_destroy(xfs5152cedev.class);
	/*解除片选gpio的占用*/
	gpio_free(xfs5152cedev.cs_gpio);
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



