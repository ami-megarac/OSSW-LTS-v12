/*****************************************************************
 **                                                             **
 **     (C) Copyright 2009-2015, American Megatrends Inc.       **
 **                                                             **
 **             All Rights Reserved.                            **
 **                                                             **
 **         5555 Oakbrook Pkwy Suite 200, Norcross,             **
 **                                                             **
 **         Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                             **
 ****************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/cacheflush.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/version.h>
#include <linux/cdev.h>
#include <linux/kfifo.h>

#include "postcode.h"
#include "postcode_ioctl.h"
#include "reset.h"
#include "driver_hal.h"

#ifdef HAVE_UNLOCKED_IOCTL  
  #if HAVE_UNLOCKED_IOCTL  
        #define USE_UNLOCKED_IOCTL  
  #endif  
#endif 

#define DRIVER_AUTHOR 	"American Megatrends Inc"
#define DRIVER_DESC 	"Postcode Common Driver"
#define DRIVER_LICENSE	"GPL"

/* Module information */
MODULE_AUTHOR( DRIVER_AUTHOR );
MODULE_DESCRIPTION( DRIVER_DESC );
MODULE_LICENSE ( DRIVER_LICENSE );

#ifdef DEBUG
#define dbgprint(fmt, args...)       printk (KERN_INFO fmt, ##args) 
#else
#define dbgprint(fmt, args...)       
#endif

#define POSTCODE_MAJOR		47
#define POSTCODE_MINOR		0
#define POSTCODE_MAX_DEVICES	255
#define POSTCODE_DEV_NAME		"postcode"

struct postcode_hal
{
    unsigned int num_instances;
    struct postcode_kfifo post_codes[POSTCODE_BUFSIZE];
};

struct postcode_dev
{
	struct postcode_hal* ppostcode_hal;
	unsigned char instance_num;
};

unsigned int *postcode_databuf;

static int __init postcode_init(void);
static void __exit postcode_exit(void);

static int postcode_open (struct inode * inode, struct file *file);
#ifdef USE_UNLOCKED_IOCTL
static long postcode_ioctlUnlocked(struct file *file, unsigned int cmd, unsigned long arg);
#else
static int postcode_ioctl (struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
#endif
static int postcode_release (struct inode *inode, struct file *file);

static int handle_postcode_reset (void);
static inline void postcode_kfifo_get (struct postcode_kfifo *fifo, unsigned int *buffer, unsigned int length);

static struct file_operations postcode_fops = 
{
	owner:		THIS_MODULE,
	open:		postcode_open,
	release:	postcode_release,
#ifdef USE_UNLOCKED_IOCTL
	unlocked_ioctl:    postcode_ioctlUnlocked,
#else 
	ioctl:             postcode_ioctl
#endif
};


static struct cdev *postcode_cdev;
static dev_t postcode_devno = MKDEV(POSTCODE_MAJOR, POSTCODE_MINOR);
static char banner[] __initdata = KERN_INFO "Postcode Common Driver, (c) 2009-2015 American Megatrends Inc.\n";


/*********************************************************************************************/

static void __exit
postcode_exit(void)
{
	unregister_core_hal_module (EDEV_TYPE_POSTCODE);
	unregister_chrdev_region (postcode_devno, POSTCODE_MAX_DEVICES);

	if (NULL != postcode_cdev)
	{
		dbgprint ("postcode char device del\n");	
		cdev_del (postcode_cdev);
	}
	printk("Postcode common module unloaded successfully\n");
	return;
}

int
register_postcode_hal_module (unsigned char num_instances, void *phal_ops, void **phw_data)
{
	struct postcode_hal *ppostcode_hal = NULL;
    int i=0;

    phal_ops=phal_ops;

	ppostcode_hal = (struct postcode_hal*) kmalloc (sizeof(struct postcode_hal), GFP_KERNEL);
	if (!ppostcode_hal)
	{
		return -ENOMEM;
	}

    for(i=0;i<num_instances;i++) {
		if (kfifo_alloc (&ppostcode_hal->post_codes[i], POSTCODE_BUFSIZE, GFP_KERNEL))
        {
            kfree (ppostcode_hal);
            return -ENOMEM;
        }

		ppostcode_hal->post_codes[i].kfifo.mask = 0;
    }

    ppostcode_hal->num_instances=num_instances;

	*phw_data = (void *)ppostcode_hal;

	install_reset_handler (handle_postcode_reset);
	return 0;
}


int
unregister_postcode_hal_module (void *phw_data)
{
	struct postcode_hal *ppostcode_hal = (struct postcode_hal *)phw_data;
    int i=0;

	dbgprint ("unregister hal addr : %p\n", ppostcode_hal);

	uninstall_reset_handler (handle_postcode_reset);
    for(i=0;i<ppostcode_hal->num_instances;i++) {
		kfifo_free (&ppostcode_hal->post_codes[i]);
	}
	kfree (ppostcode_hal);
	return 0;
}


static int 
postcode_open (struct inode * inode, struct file * file)
{
	struct postcode_hal *ppostcode_hal;
	unsigned int minor = iminor(inode);
	struct postcode_dev* pdev;
	hw_info_t postcode_hw_info;
	int ret;
	unsigned char open_count;

	ret = hw_open (EDEV_TYPE_POSTCODE, minor, &open_count, &postcode_hw_info);
	if (ret)
		return -ENXIO;

	ppostcode_hal = postcode_hw_info.pdrv_data;

	pdev = (struct postcode_dev*) kmalloc (sizeof(struct postcode_dev), GFP_KERNEL);
	if (!pdev)
	{
		hw_close (EDEV_TYPE_POSTCODE, minor, &open_count);
		printk (KERN_ERR "%s: failed to allocate postcode private dev structure for postcode iminor: %d\n", POSTCODE_DEV_NAME, minor);
		return -ENOMEM;
	}

	pdev->ppostcode_hal = ppostcode_hal;
	pdev->instance_num = postcode_hw_info.inst_num;
	file->private_data = pdev;
	dbgprint ("%d, postcode_open priv data addr : %p\n", minor, &file->private_data);
	return nonseekable_open (inode, file);
}

static int postcode_release (struct inode *inode, struct file *file)
{
	int ret;
	unsigned char open_count;
	
	struct postcode_dev *pdev = (struct postcode_dev*) file->private_data;
	pdev->ppostcode_hal = NULL;
	ret = hw_close (EDEV_TYPE_POSTCODE, iminor(inode), &open_count);
	if(ret) { return -1; }
	kfree (pdev);
	return 0;
}


static long
postcode_ioctlUnlocked (struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct postcode_dev* pdev = (struct postcode_dev*) file->private_data;
	unsigned char* buf = (unsigned char*) arg;
	unsigned int size = 0;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,0,0))
	unsigned int minor = iminor(file->f_dentry->d_inode);
#else
	unsigned int minor = iminor(file->f_path.dentry->d_inode);
#endif

	switch (cmd)
	{

		case READ_POST_CODES:
            dbgprint ("BR kfifo %d: %d, %d, %d\n", minor, pdev->ppostcode_hal->post_codes[minor].kfifo.in , 
            pdev->ppostcode_hal->post_codes[minor].kfifo.out , pdev->ppostcode_hal->post_codes[minor].kfifo.mask );
            size = pdev->ppostcode_hal->post_codes[minor].kfifo.mask;
            postcode_kfifo_get ( &pdev->ppostcode_hal->post_codes[minor], postcode_databuf, size );
            dbgprint ("AR kfifo %d: %d, %d, %d\n", minor, pdev->ppostcode_hal->post_codes[minor].kfifo.in , 
            pdev->ppostcode_hal->post_codes[minor].kfifo.out , pdev->ppostcode_hal->post_codes[minor].kfifo.mask );


#ifdef CONFIG_SPX_FEATURE_POSTCODE_CLEAR_DATA_SUPPORT
            pdev->ppostcode_hal->post_codes[minor].kfifo.mask = 0;
            pdev->ppostcode_hal->post_codes[minor].kfifo.out = pdev->ppostcode_hal->post_codes[minor].kfifo.in;
#endif
			if ((ret = __copy_to_user( (unsigned char*) (buf), (unsigned char*) postcode_databuf, sizeof(unsigned int) * size )) != 0)
			{
				printk ("READ_POST_CODES: Error copying data to user \n");
				return ret;
			}
			break;

		default:
			printk("unrecognized IOCTL call\n");
			return -1;
			break;
	}

	return size;
}

#ifndef USE_UNLOCKED_IOCTL  
static int
postcode_ioctl (struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    return ( postcode_ioctlUnlocked (file, cmd, arg) );
}
#endif

static int
handle_postcode_reset (void)
{
	int i = 0, j=0;
	struct postcode_hal *ppostcode_hal;

	for (i=0; i< POSTCODE_MAX_DEVICES; i++)
	{
		ppostcode_hal = hw_intr (EDEV_TYPE_POSTCODE, i);
		if (!ppostcode_hal)
			continue;

        for (j=0; j<ppostcode_hal->num_instances; j++) {
			ppostcode_hal->post_codes[j].kfifo.mask = 0;
			ppostcode_hal->post_codes[j].kfifo.in = 0;
			ppostcode_hal->post_codes[j].kfifo.out = 0;
        }
	}
	return 0;
}

void
get_postcode_core_data ( struct postcode_kfifo ** ppost_codes, int dev_id, int ch_num )
{
	struct postcode_hal *ppostcode_hal;

	ppostcode_hal = hw_intr (EDEV_TYPE_POSTCODE, dev_id);
	*ppost_codes = &ppostcode_hal->post_codes[ch_num];
}

static void
postcode_kfifo_get (struct postcode_kfifo *fifo, unsigned int *buffer, unsigned int length)
{
	unsigned int l;
	if ( fifo->kfifo.in < fifo->kfifo.out )
	{
		l = fifo->kfifo.mask - fifo->kfifo.out;
		memcpy ( buffer, fifo->kfifo.data + (fifo->kfifo.out - 1)%POSTCODE_BUFSIZE , l+1 );
		memcpy ( &buffer[l+1], fifo->kfifo.data, fifo->kfifo.out-1 );
	}
	else
		memcpy ( buffer, fifo->kfifo.data + fifo->kfifo.out, sizeof(unsigned int) * (fifo->kfifo.in - fifo->kfifo.out) );
}


/* ------------------- Driver registration ------------------------------- */

static postcode_core_funcs_t postcode_core_funcs = {
	.get_postcode_core_data = get_postcode_core_data,
};

static core_hal_t postcode_core_hal = {
        .owner                 = THIS_MODULE,
        .name                  = "POSTCODE CORE",
        .dev_type              = EDEV_TYPE_POSTCODE,
        .register_hal_module   = register_postcode_hal_module,
        .unregister_hal_module = unregister_postcode_hal_module,
        .pcore_funcs           = (void *)&postcode_core_funcs
};


static int __init
postcode_init(void)
{
	int ret = 0;
	printk (banner);

	if ((ret = register_chrdev_region (postcode_devno, POSTCODE_MAX_DEVICES, POSTCODE_DEV_NAME)) < 0)
	{
		printk (KERN_ERR "failed to register postcode device <%s> (err: %d)\n", POSTCODE_DEV_NAME, ret);
		return ret;
	}

	postcode_cdev = cdev_alloc();
	if (!postcode_cdev)
	{
		printk (KERN_ERR "%s: failed to allocate postcode cdev structure\n", POSTCODE_DEV_NAME);
		unregister_chrdev_region (postcode_devno, POSTCODE_MAX_DEVICES);
		return -1;
	}

	cdev_init (postcode_cdev, &postcode_fops);

	if ((ret = cdev_add (postcode_cdev, postcode_devno, POSTCODE_MAX_DEVICES)) < 0)
	{
		printk  (KERN_ERR "failed to add <%s> char device\n", POSTCODE_DEV_NAME);
		cdev_del (postcode_cdev);
		unregister_chrdev_region (postcode_devno, POSTCODE_MAX_DEVICES);
		ret=-ENODEV;	
		return ret;
	}

    postcode_databuf = (unsigned int*)kmalloc(sizeof(unsigned int) * POSTCODE_BUFSIZE, GFP_KERNEL);

	ret = register_core_hal_module (&postcode_core_hal);
	if (ret < 0)
	{
		cdev_del (postcode_cdev);
		unregister_chrdev_region (postcode_devno, POSTCODE_MAX_DEVICES);
		printk("error in registering the core postcode module\n");
		ret=-EINVAL;
		return ret;
	}
	return ret;
}


module_init (postcode_init);
module_exit (postcode_exit);

int postcode_core_loaded =1;
EXPORT_SYMBOL(postcode_core_loaded);
