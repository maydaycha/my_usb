#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include "usb.h"

#define NAME    "zebu-char"

#define MAX_CHAR_DEV      6
#define DYNAMIC_MINORS    (1 << MAX_CHAR_DEV)  /* like dynamic majors. 6 bits usable for maximun 6 device nodes */
static DECLARE_BITMAP(char_minors, DYNAMIC_MINORS);

static dev_t rtk_usb_dev_t = 0;
static struct class *rtk_usb_class = NULL;


extern int alloc_chrdev_region(dev_t *dev, unsigned baseminor, unsigned count, const char *name);
extern void unregister_chrdev_region(dev_t from, unsigned count);


static int char_open(struct inode *inode, struct file *filp)
{
    struct usb_zebu_data *zebu = cdev_to_zebu(inode->i_cdev);

    if (zebu->current_urb)
        return -EBUSY;

    zebu->current_urb = usb_alloc_urb(0, GFP_KERNEL);
    if (!zebu->current_urb)
        return -ENOMEM;

    usb_zebu_err(zebu, " %s call \n", __func__);

    return 0;
}


static int char_release(struct inode *inode, struct file *filp)
{
    struct usb_zebu_data *zebu = cdev_to_zebu(inode->i_cdev);

    if (zebu->current_urb) {
        usb_free_urb(zebu->current_urb);
        zebu->current_urb = NULL;
    }

    usb_zebu_err(zebu, " %s call \n", __func__);

    return 0;
}


static ssize_t char_read(struct file *filp, char __user *buffer, size_t count, loff_t *f_ops)
{
    struct usb_zebu_data *zebu = cdev_to_zebu(filp->f_inode->i_cdev);

    usb_zebu_err(zebu, " %s call \n", __func__);
    return 0;
}



static void zebu_urb_blocking_completion(struct urb *urb)
{
    struct completion *urb_done_ptr = urb->context;

    dev_err(&urb->dev->dev, "call %s \n", __func__);
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



static ssize_t char_write(struct file *filp, const char __user *buffer, size_t count, loff_t *f_ops)
{
    struct usb_zebu_data *zebu = cdev_to_zebu(filp->f_inode->i_cdev);
    int result = 0;
    char *buf = kzalloc(count, GFP_KERNEL);

    if (unlikely(!buf || !zebu->current_urb))
        return -ENOMEM;

    if (unlikely(copy_from_user(buf, buffer, count))) {
        result = -EINVAL;
        goto out;
    }

    usb_zebu_err(zebu, "write: %s \n", buf);
    usb_zebu_err(zebu, "count: %u \n", count);

    /* fill and submit the URB */
    usb_fill_bulk_urb(zebu->current_urb, zebu->udev, zebu->send_bulk_pipe, buf, count,
                zebu_urb_blocking_completion, NULL);
    result = zebu_msg_common(zebu, 0);

out:
    kfree(buf);
    usb_zebu_err(zebu, "write complete. result=%d. data length =%d  \n", result, zebu->current_urb->actual_length);
    return result ? result : zebu->current_urb->actual_length;
}



struct file_operations zebu_chardev_fops = {
    .owner            = THIS_MODULE,
    .open             = char_open,
    .release          = char_release,
    .read             = char_read,
    .write            = char_write,
};


int rtk_usb_cdev_create(struct usb_zebu_data *zebu)
{
    int ret = 0;
    char name[50];
    unsigned long cleared_bit = 0, minor = 0;

    cleared_bit = find_first_zero_bit(char_minors, DYNAMIC_MINORS);
    if (cleared_bit >= DYNAMIC_MINORS) /* out of range */
        return -EBUSY;
    minor = DYNAMIC_MINORS - cleared_bit - 1;
    set_bit(cleared_bit, char_minors);

    cdev_init(&zebu->char_dev, &zebu_chardev_fops);
    ret = cdev_add(&zebu->char_dev, MKDEV(MAJOR(rtk_usb_dev_t), minor), 1);
    if (ret) {
        usb_zebu_err(zebu, "fail to add char dev to system \n");
        return -ENODEV;
    }

    if (snprintf(name, sizeof(name), "zebu-%s", dev_name(&zebu->udev->dev)) <= 0)
        return -ENOMEM;

    if (!device_create(rtk_usb_class, NULL, zebu->char_dev.dev, zebu, name)) {
        usb_zebu_err(zebu, "fail to cearte device in sysfs \n");
        ret = -ENOMEM;
        goto FAIL_CREATE_DEV;
    }

    return ret;

FAIL_CREATE_DEV:
    cdev_del(&zebu->char_dev);

    return ret;
}
EXPORT_SYMBOL(rtk_usb_cdev_create);


void rtk_usb_cdev_destroy(struct usb_zebu_data *zebu)
{
    device_destroy(rtk_usb_class, zebu->char_dev.dev);
    cdev_del(&zebu->char_dev);

    usb_zebu_err(zebu, "%s \n", __func__);
}
EXPORT_SYMBOL(rtk_usb_cdev_destroy);



int __init init_class_devt_region(void)
{
    int ret = 0;

    ret = alloc_chrdev_region(&rtk_usb_dev_t, 0, MAX_CHAR_DEV, NAME);
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
    unregister_chrdev_region(rtk_usb_dev_t, MAX_CHAR_DEV);
    return ret;
}


void __exit exit_class_devt_region(void)
{
    class_destroy(rtk_usb_class);
    rtk_usb_class = NULL;
    unregister_chrdev_region(rtk_usb_dev_t, MAX_CHAR_DEV);
}



module_init(init_class_devt_region);
module_exit(exit_class_devt_region);
