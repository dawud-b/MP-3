
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

MODULE_DESCRIPTION("Launcher Module");
MODULE_AUTHOR("Zane and Dawud");
MODULE_LICENSE("GPL");

struct usb_launcher {
    struct usb_device* udev;
    struct usb_interface* interface;
    struct urb* interrupt_in_urb;
    size_t interrupt_in_buffer_size;
    unsigned char* interrupt_in_buffer;
    struct usb_endpoint_descriptor* interrupt_in_endpoint;
    struct kref kref;
    struct mutex io_mutex;
    spinlock_t cmd_spinlock;
    unsigned char command;
};

static struct usb_device_id_launcher_table[] = {
    { USB_DEVICE(LAUNCHER_VENDOR_ID, LAUNCHER_PRODUCT_ID) },
    { }
};

static const struct file_operations launcher_fops = {
    .owner = THIS_MODULE,
    .write = launcher_write,
    .open = launcher_open,
    .release = launcher_release,
};

static struct usb_class_driver launcher_class = {
    .name = "launcher%d",
    .fops = &launcher_fops,
};



static int launcher_probe(struct usb_interface *interface, const struct usb_device_id *id) {
    printk("Launcher Probe: USB device connected")

    struct usb_device* udev = interface_to_usbdev(interface);
    struct usb_launcher* dev = NULL;
    struct usb_host_interface* iface_desc;
    struct usb_endpoint_descriptor *endpoint;
    int retval = -ENODEV;

    if (!udev) {
        pr_err("usb is null");
        goto exit;
    }

    dev = kzalloc(sizeof(struct usb_launcher), GFP_KERNEL);

    dev->command = LAUNCHER_STOP;

    mutex_init(&dev->io_mutex);
    spin_lock_init(&dev->cmd_spinlock);

    dev->udev = udev;
    dev->interface = interface;
    iface_desc = interface->cur_altsetting;

    for (int i = 0; i < iface_desc->desc.bNumEndpoints; i++) {
        endpoint = &iface_desc->endpoint[i].desc;

        if (((endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN) && ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT)) {
            dev->interrupt_in_endpoint = endpoint;
            size_t buffer_size = usb_endpoint_maxp(endpoint);
            dev->interrupt_in_buffer_size = buffer_size;
            dev->interrupt_in_buffer = kzmalloc(buffer_size, GFP_KERNEL);
            if (!dev->interrupt_in_buffer) {
                pr_err("Couldn't allocate interrupt in buffer");
                goto exit;
            }

            dev->interrupt_in_urb = usb_alloc_urb(0, GFP_KERNEL);
            if (!dev->interrupt_in_urb) {
                pr_err("Couldn't allocate interrupt in urb");
                goto exit;
            }
        }
    }

    if (!dev->interrupt_in_endpoint) {
        pr_err("Couldn't find interrupt endpoint");
        goto exit;
    }

    usb_set_intfdata(interface, dev);

    usb_fill_int_urb(dev->interrupt_in_urb, dev->udev, usb_rcvintpipe(dev->udev, dev->interrupt_in_endpoint.bEndpointAddress),
                     dev->interrupt_in_buffer, dev->interrupt_in_buffer_size, launcher_interrupt_in_callback, dev->interrupt_in_endpoint.bInterval);
    

    retval = usb_register_dev(interface, &launcher_class);





    
    exit:
    return retval;

}

static int launcher_open(struct inode *inode, struct file *file) {
    struct usb_launcher* dev;
    int subminor = iminor(inode);
    struct usb_interface* interface = usb_find_interface(&launcher_driver, subminor);
    int retval = 0;

    if (!interface) {
        pr_err("%s - error, can't find device for minor %d\n", __func__, subminor);
        retval = -ENODEV;
        goto exit;
    }

    dev = usb_get_intfdata(interface);
    if (!dev) {
        retval = -ENODEV;
        return exit;
    }

    retval = usb_autopm_get_interface(interface);
    if (retval)
        goto exit;

    kref_get(&dev->kref);

    file->private_data = dev;
    

    exit:
    return retval;
    
}

#define to_launcher_dev(d) container_of(d, struct usb_launcher, kref)

static void launcher_delete(struct kref* kref) {
    struct usb_launcher* dev = to_launcher_dev(kref);
    usb_free_urb(dev->interrupt_in_urb)
}

static int launcher_release(struct inode *inode, struct file *file) {
    struct usb_launcher* dev = file->private_data;

    if (!dev)
        return -ENODEV;

    mutex_lock(&dev->io_mutex);
    if (dev->interface)
        usb_autopm_put_interface(dev->interface);
    mutex_unlock(&dev->io_mutex);

    kref_put(&dev->kref, )
    

}





static int launcher_disconnect(struct usb_interface* interface) {

    
}

static void launcher_interrupt_in_callback(struct urb* urb, struct usb_launcher* dev) {

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


static struct usb_driver launcher_driver = {
    .name = "launcher",
    .probe = launcher_probe,
    .disconnect = launcher_disconnect,
    .id_table = launcher_table
};

module_usb_driver(launcher_driver);
