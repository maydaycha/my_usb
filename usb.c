#include <linux/module.h>
#include <linux/slab.h>
#include <linux/usb.h>
#include "usb.h"

#define DRV_NAME    "usb-zebu"

MODULE_AUTHOR("Jason Chiu <jason.chiu@realtek.com>");
MODULE_LICENSE("GPL");


static int get_pipes(struct usb_zebu_data *zebu)
{
    struct usb_host_interface *alt = zebu->intf->cur_altsetting;
    struct usb_endpoint_descriptor *ep_blk_in;
    struct usb_endpoint_descriptor *ep_blk_out;
    int res;

    /*
    * Find the first endpoint of each type we need.
    * We are expecting a minimum of 2 endpoints - in and out (bulk).
    * An optional interrupt-in is OK (necessary for CBI protocol).
    * We will ignore any others.
    */
    res = usb_find_common_endpoints(alt, &ep_blk_in, &ep_blk_out, NULL, NULL);
    if (res) {
        usb_zebu_dbg(zebu, "bulk endpoints not found\n");
        return res;
    }

    /* Calculate and store the pipe values */
    zebu->send_ctrl_pipe = usb_sndctrlpipe(zebu->udev, 0);
    zebu->recv_ctrl_pipe = usb_rcvctrlpipe(zebu->udev, 0);
    zebu->send_bulk_pipe = usb_sndbulkpipe(zebu->udev,
        usb_endpoint_num(ep_blk_out));
    zebu->recv_bulk_pipe = usb_rcvbulkpipe(zebu->udev,
        usb_endpoint_num(ep_blk_in));

    return 0;
}


static int usb_zebu_probe(struct usb_interface *intf,
        const struct usb_device_id *id)
{
    struct usb_zebu_data *zebu;
    int ret = 0;

    zebu = kzalloc(sizeof(struct usb_zebu_data), GFP_KERNEL);
    if (!zebu)
        return -ENODEV;

    zebu->udev = interface_to_usbdev(intf);
    zebu->intf = intf;

    ret = get_pipes(zebu);
    if (ret)
        goto probe_fail;

    usb_set_intfdata(intf, zebu);

    dev_err(&zebu->udev->dev, "call %s \n", __func__);
    dev_err(&zebu->intf->dev, "call %s \n", __func__);

    return 0;

probe_fail:
    kfree(zebu);
    return ret;
}


static void usb_zebu_disconnect(struct usb_interface *intf)
{
    struct usb_zebu_data *zebu = usb_get_intfdata(intf);

    dev_err(&zebu->udev->dev, "call %s \n", __func__);
    dev_err(&zebu->intf->dev, "call %s \n", __func__);
}


static int usb_zebu_suspend(struct usb_interface *intf, pm_message_t message)
{
    struct usb_zebu_data *zebu = usb_get_intfdata(intf);

    dev_err(&zebu->udev->dev, "call %s \n", __func__);
    dev_err(&zebu->intf->dev, "call %s \n", __func__);

    return 0;
}


static int usb_zebu_resume(struct usb_interface *intf)
{
    struct usb_zebu_data *zebu = usb_get_intfdata(intf);

    dev_err(&zebu->udev->dev, "call %s \n", __func__);
    dev_err(&zebu->intf->dev, "call %s \n", __func__);

    return 0;
}

#define USB_SC_SCSI 0x06     /* Transparent */
#define USB_PR_BULK 0x50     /* bulk only */
static struct usb_device_id usb_zebu_usb_ids[] = {
    { USB_DEVICE(0x0499, 0x3002) }, /* Match via VID, PID */
    { USB_INTERFACE_INFO(USB_CLASS_MASS_STORAGE, USB_SC_SCSI, USB_PR_BULK) },
    {},
};

static struct usb_driver usb_zebu_driver = {
    .name = DRV_NAME,
    .probe = usb_zebu_probe,
    .disconnect = usb_zebu_disconnect,
    .suspend = usb_zebu_suspend,
    .resume = usb_zebu_resume,
    .id_table = usb_zebu_usb_ids,
};


static int __init usb_zebu_driver_init(void)
{
    return usb_register(&usb_zebu_driver);
}


static void __exit usb_zebu_driver_exit(void)
{
    usb_deregister(&usb_zebu_driver);
}

module_init(usb_zebu_driver_init);
module_exit(usb_zebu_driver_exit);
