#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include "usb.h"

#define RTK_USB_DEV_NUM  16
#define NAME    "zebu-char"

static int dev_cnt = 0;
static dev_t rtk_usb_dev_t = 0;
static struct cdev rtk_usb_cdev;
static struct class *rtk_usb_class = NULL;


extern int alloc_chrdev_region(dev_t *dev, unsigned baseminor, unsigned count, const char *name);
extern void unregister_chrdev_region(dev_t from, unsigned count);


static int rtk_usb_open(struct inode *inode, struct file *filp)
{
    pr_err("inode(%p), filp->f_inode(%p) \n",
            inode, filp->f_inode);
    return 0;
}

static int rtk_usb_release(struct inode *inode, struct file *filp)
{
    pr_err("inode(%p), filp->f_inode(%p) \n",
            inode, filp->f_inode);

    return 0;
}


static ssize_t rtk_usb_read(struct file *filp, char __user *buffer, size_t count, loff_t *f_ops)
{
    pr_err("rtk_usb_cdev(%p), filp->f_inode->i_cdev(%p) \n",
            &rtk_usb_cdev, filp->f_inode->i_cdev);

    return 0;
}



static void zebu_urb_blocking_completion(struct urb *urb)
{
    struct completion *urb_done_ptr = urb->context;

    complete(urb_done_ptr);
}

static int zebu_msg_common(struct usb_zebu_data *zebu, int timeout)
{
    struct completion urb_done;
    long timeleft;
    int status;

    /* set up data structures for the wakeup system */
    init_completion(&urb_done);

    /* fill the common fields in the URB */
    zebu->current_urb->context = &urb_done;
    zebu->current_urb->transfer_flags = 0;

    /* submit the URB */
    status = usb_submit_urb(zebu->current_urb, GFP_NOIO);
    if (status) {
        /* something went wrong */
        return status;
    }

    /* wait for the completion of the URB */
    timeleft = wait_for_completion_interruptible_timeout(
            &urb_done, timeout ? : MAX_SCHEDULE_TIMEOUT);

    if (timeleft <= 0) {
        usb_zebu_warn(zebu, "%s -- cancelling URB\n",
                timeleft == 0 ? "Timeout" : "Signal");
        usb_kill_urb(zebu->current_urb);
    }

    /* return the URB status */
    return zebu->current_urb->status;
}



static ssize_t rtk_usb_write(struct file *filp, const char __user *buffer, size_t count, loff_t *f_ops)
{
    struct usb_zebu_data *zebu = cdev_to_zebu(filp->f_inode->i_cdev);
    int result = 0;
    char *buf = kzalloc(count, GFP_KERNEL);

    if (!buf)
        return -ENOMEM;

    if (unlikely(copy_from_user(buf, buffer, count)))
        return -EINVAL;

    pr_err("write: %s \n", buf);
    pr_err("count: %u \n", count);

    zebu->current_urb = usb_alloc_urb(0, GFP_KERNEL);
    if (!zebu->current_urb)
        return -ENOMEM;

    /* fill and submit the URB */
    usb_fill_bulk_urb(zebu->current_urb, zebu->udev, zebu->send_bulk_pipe, buf, count,
                zebu_urb_blocking_completion, NULL);
    result = zebu_msg_common(zebu, 0);

    return result ? result : zebu->current_urb->actual_length;
}



struct file_operations zebu_chardev_fops = {
    .owner            = THIS_MODULE,
    .open             = rtk_usb_open,
    .release          = rtk_usb_release,
    .read             = rtk_usb_read,
    .write            = rtk_usb_write,
};


int rtk_usb_cdev_init(struct usb_zebu_data *zebu)
{
    int ret = 0;
    char name[50];

    pr_err("zebu(%p) \n", zebu);

    if (snprintf(name, sizeof(name), "zebu-%s", dev_name(&zebu->udev->dev)) <= 0)
        return -ENOMEM;

    cdev_init(&zebu->char_dev, &zebu_chardev_fops);
    ret = cdev_add(&zebu->char_dev, rtk_usb_dev_t + dev_cnt, 1);
    if (ret) {
        pr_err("fail to add char dev to system \n");
        return -ENODEV;
    }

    dev_cnt++;

    if (!device_create(rtk_usb_class, NULL, zebu->char_dev.dev, zebu, name)) {
        pr_err("fail to cearte device node for rtk_usb \n");
        ret = -ENOMEM;
        goto FAIL_CREATE_DEV;
    }

    return ret;

FAIL_CREATE_DEV:
    cdev_del(&rtk_usb_cdev);
    dev_cnt--;

    return ret;
}
EXPORT_SYMBOL(rtk_usb_cdev_init);


int __init init_class_devt_region(void)
{
    int ret = 0;

    ret = alloc_chrdev_region(&rtk_usb_dev_t, 0, RTK_USB_DEV_NUM, NAME);
    if (ret) {
        pr_err("fail to get char dev Major and Minor \n");
        return -ENOMEM;
    }

    rtk_usb_class = class_create(THIS_MODULE, NAME);
    if (IS_ERR(rtk_usb_class)) {
        pr_err("fail to cearte class \n");
        ret = PTR_ERR(rtk_usb_class);
        goto FAIL_CREATE_CLASS;
    }

    return 0;

FAIL_CREATE_CLASS:
    unregister_chrdev_region(rtk_usb_dev_t, RTK_USB_DEV_NUM);
    return ret;
}


void __exit exit_class_devt_region(void)
{
    class_destroy(rtk_usb_class);
    rtk_usb_class = NULL;
    unregister_chrdev_region(rtk_usb_dev_t, RTK_USB_DEV_NUM);
}



module_init(init_class_devt_region);
module_exit(exit_class_devt_region);
