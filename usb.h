#ifndef __ZEBU_USB_H__
#define __ZEBU_USB_H__
#include <linux/usb.h>
#include <linux/cdev.h>


struct usb_zebu_data {
    struct usb_device *udev;       /* this usb_device */
    struct usb_interface *intf;    /* this interface */
    unsigned int send_bulk_pipe;   /* cached pipe values */
    unsigned int recv_bulk_pipe;
    unsigned int send_ctrl_pipe;
    unsigned int recv_ctrl_pipe;
    struct urb *current_urb;
    struct cdev char_dev;
};

#define usb_zebu_dbg(zebu, fmt, args...) \
        dev_dbg(&zebu->udev->dev, fmt, ## args)
#define usb_zebu_warn(zebu, fmt, args...) \
        dev_warn(&zebu->udev->dev, fmt, ## args)
#define usb_zebu_err(zebu, fmt, args...) \
        dev_err(&zebu->udev->dev, fmt, ## args)


static inline struct usb_zebu_data *cdev_to_zebu(struct cdev *cdev)
{
    return container_of(cdev, struct usb_zebu_data, char_dev);
}

extern int rtk_usb_cdev_create(struct usb_zebu_data *zebu);
extern void rtk_usb_cdev_destroy(struct usb_zebu_data *zebu);

#endif
