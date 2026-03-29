
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

MODULE_DESCRIPTION("Launcher Module");
MODULE_AUTHOR("Zane and Dawud");
MODULE_LICENSE("GPL");

struct usb_launcher {

};

static struct usb_device_id_launcher_table[] = {
    { USB_DEVICE(LAUNCHER_VENDOR_ID, LAUNCHER_PRODUCT_ID) },
    { }
};

static int launcher_open(struct inode *inode, struct file *file) {

}

static int launcher_release(struct inode *inode, struct file *file) {

}




static int launcher_probe(struct usb_interface *interface, const struct usb_device_id *id) {
    printk("Launcher Probe: USB device connected")

    struct usb_device* udev = interface_to_usb(interface)
    struct usb_launcher* launch_context = NULL;
    struct usb_host_interface* iface_desc;
    struct usb_endpoint_descriptor* endpoint;
    

}

static int __init launcher_init(void) {
    pr_info("Launcher init\n");
    return 0;
}

static int __exit launcher_exit(void) {
    pr_info("Launcher exit\n");
    return 0;
}

module_init(launcher_init);
module_exit(launcher_exit);
