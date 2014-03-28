#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>

#include "ktoy.h"

dev_t devno;

static int ktoy_val;

long ktoy_ioctl(struct file *inode, unsigned int cmd, unsigned long arg)
{
        int err = 0;
        int retval = 0;

        /* make sure the command belongs to this module */
        if (_IOC_TYPE(cmd) != KTOY_IOC_MAGIC) return -ENOTTY;
        /* make sure command number is in valid range */
	if (_IOC_NR(cmd) > KTOY_IOC_MAXNR) return -ENOTTY;

        /* verify user space pointer */
        if (_IOC_DIR(cmd) & _IOC_WRITE)
                err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
        else if (_IOC_DIR(cmd) & _IOC_READ)
                err = !access_ok(VERIFY_Write, (void __user *)arg, _IOC_SIZE(cmd));
        if (err) {
		printk(KERN_INFO "error with cmd");
                return -EFAULT;
	}

        switch(cmd) {
        case KTOY_IOC_SET:
                printk(KERN_INFO "ioctl SET called\n");
                retval = __get_user(ktoy_val, (int __user *)arg);
                break;
        case KTOY_IOC_GET:
                printk(KERN_INFO "ioctl GET called\n");
                retval = __put_user(ktoy_val, (int __user *)arg);
                break;
        }
        return retval;
}

struct ktoy_dev {
        struct cdev cdev;
};

struct ktoy_dev ktoy_dev_entity;
struct class *cl;

struct file_operations ktoy_fops = {
        .owner = THIS_MODULE,
        /* .llseek = ktoy_llseek, */
        /* .read = ktoy_read, */
        /* .write = ktoy_write, */
        .unlocked_ioctl = ktoy_ioctl,
        /* .open = ktoy_open, */
        /* .release = ktoy_release, */
};

/* use cdev interface to associate device number with a device */
static void ktoy_setup_cdev(struct ktoy_dev *dev)
{
        int err;

        /* init chrdev with a set of file_operatoins */
        cdev_init(&dev->cdev, &ktoy_fops);
        dev->cdev.owner = THIS_MODULE;

        /* add cdev to the system, associating it with the devno */
        err = cdev_add(&dev->cdev, devno, 1);
        if (err) {
                printk(KERN_ERR "fail to add ktoy\n");
        }
}

static int __init ktoy_init(void)
{
        int retval;

        if ((retval = alloc_chrdev_region(&devno, 0 /* firstminor */, 1 /* count */, "ktoy")) != 0) {
                printk(KERN_ERR "fail to allocate character device\n");
                return retval;
        }
        ktoy_setup_cdev(&ktoy_dev_entity);

        cl = class_create(THIS_MODULE, "ktoy");
        if (IS_ERR(cl)) {
                printk(KERN_ERR "class_create failed\n");
                return PTR_ERR(cl);
        }

        device_create(cl, NULL, devno, NULL, "ktoy");
        ktoy_val = 0xdeadbeef;
        printk(KERN_INFO "hello, world2\n");
        return retval;
}

static void __exit ktoy_exit(void)
{
        cdev_del(&ktoy_dev_entity.cdev);

        device_destroy(cl, devno);
        class_destroy(cl);

        unregister_chrdev_region(devno, 1);
        printk(KERN_INFO "bye world\n");
}

MODULE_LICENSE("GPL");

module_init(ktoy_init);
module_exit(ktoy_exit);
