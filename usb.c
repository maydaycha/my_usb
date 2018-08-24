#include <linux/module.h>
#include <linux/usb.h>

#define DRV_NAME    "usb-zebu"

MODULE_AUTHOR("Jason Chiu <jason.chiu@realtek.com>");
MODULE_LICENSE("GPL");


static int usb_zebu_probe(struct usb_interface *intf,
        const struct usb_device_id *id)
{
    struct usb_device *udev = interface_to_usbdev(intf);

    dev_err(&udev->dev, "call %s \n", __func__);
    dev_err(&intf->dev, "call %s \n", __func__);

    return 0;
}


static void usb_zebu_disconnect(struct usb_interface *intf)
{
    struct usb_device *udev = interface_to_usbdev(intf);

    dev_err(&udev->dev, "call %s \n", __func__);
    dev_err(&intf->dev, "call %s \n", __func__);
}

static int usb_zebu_suspend(struct usb_interface *intf, pm_message_t message)
{
    struct usb_device *udev = interface_to_usbdev(intf);

    dev_err(&udev->dev, "call %s \n", __func__);
    dev_err(&intf->dev, "call %s \n", __func__);

    return 0;
}


static int usb_zebu_resume(struct usb_interface *intf)
{
    struct usb_device *udev = interface_to_usbdev(intf);

    dev_err(&udev->dev, "call %s \n", __func__);
    dev_err(&intf->dev, "call %s \n", __func__);

    return 0;
}

static struct usb_device_id usb_zebu_usb_ids[] = {
    { USB_DEVICE(0x0499, 0x3002) }, /* Match via VID, PID */
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
    printk(" %s \n", __func__);

    return usb_register(&usb_zebu_driver);
}


static void __exit usb_zebu_driver_exit(void)
{
    printk(" %s \n", __func__);
    usb_deregister(&usb_zebu_driver);
}

module_init(usb_zebu_driver_init);
module_exit(usb_zebu_driver_exit);
