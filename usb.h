#include <linux/usb.h>


struct usb_zebu_data {
    struct usb_device *udev;       /* this usb_device */
    struct usb_interface *intf;    /* this interface */
    unsigned int send_bulk_pipe;   /* cached pipe values */
    unsigned int recv_bulk_pipe;
    unsigned int send_ctrl_pipe;
    unsigned int recv_ctrl_pipe;
};

#define usb_zebu_dbg(zebu, fmt, args...) \
        dev_dbg(&zebu->udev->dev, fmt, ## args)
#define usb_zebu_warn(zebu, fmt, args...) \
        dev_warn(&zebu->udev->dev, fmt, ## args)
#define usb_zebu_err(zebu, fmt, args...) \
        dev_err(&zebu->udev->dev, fmt, ## args)
