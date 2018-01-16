/* The function hid_irq_in(struct urb *urb) is where the magic is.
 * For more ideas, look in include/linux/usb.h for attributes
 * of the "struct urb" structure, since that's how USB transfers data. */

// printk("urb->transfer_buffer_length = %d\n", urb->transfer_buffer_length); 	/* Always returns 8 */
// printk("urb->transfer_buffer = %p\n", urb->transfer_buffer);			/* This is the thing to fuck with! */

#include <linux/module.h>
#include <linux/usb.h>
#include "hid.h"


int hid_input_report(struct hid_device *, int type, u8 *, int, int);
struct hid_device *hid_allocate_device(void);
int hid_parse_report(struct hid_device *hid, __u8 *start, unsigned size);     


/*  API provided by hid-core.c for USB HID drivers */
void usbhid_close(struct hid_device *hid);
int  usbhid_open(struct hid_device *hid);

/* USB-specific HID struct, to be pointed to from struct hid_device->driver_data */
struct usbhid_device {
	struct hid_device *hid;						/* pointer to corresponding HID dev */

	struct usb_interface *intf;                                     /* USB interface */
	int ifnum;                                                      /* USB interface number */

	unsigned int bufsize;                                           /* URB buffer size */

	struct urb *urbin;                                              /* Input URB */
	char *inbuf;                                                    /* Input buffer */
	dma_addr_t inbuf_dma;                                           /* Input buffer dma */

	struct urb *urbctrl;                                            /* Control URB */
	struct usb_ctrlrequest *cr;                                     /* Control request struct */
	char *ctrlbuf;                                                  /* Control buffer */

	unsigned long iofl;                                             /* I/O flags (CTRL_RUNNING, OUT_RUNNING) */
};

#define	hid_to_usb_dev(hid_dev) \
	container_of(hid_dev->dev.parent->parent, struct usb_device, dev)


/* Input interrupt completion handler. */
static void hid_irq_in(struct urb *urb)
{
	hid_input_report(urb->context, 0, urb->transfer_buffer, urb->actual_length, 1);
	usb_submit_urb(urb, GFP_ATOMIC);
}

/* Control pipe completion handler. */
static void hid_ctrl(struct urb *urb)
{
	struct hid_device *hid = urb->context;
	struct usbhid_device *usbhid = hid->driver_data;
	clear_bit(1, &usbhid->iofl);
	usb_autopm_put_interface_async(usbhid->intf);
}

int usbhid_open(struct hid_device *hid)
{
	struct usbhid_device *usbhid = hid->driver_data;
	hid->open++;
	usb_submit_urb(usbhid->urbin, GFP_ATOMIC);
	clear_bit(3, &usbhid->iofl);
	usb_autopm_put_interface(usbhid->intf);
	return 0;
}

void usbhid_close(struct hid_device *hid)
{
	struct usbhid_device *usbhid = hid->driver_data;
	usb_kill_urb(usbhid->urbin);
	usbhid->intf->needs_remote_wakeup = 0;
}


/* Traverse the supplied list of reports and find the longest */
static void hid_find_max_report(struct hid_device *hid, unsigned int type, unsigned int *max)
{
	struct hid_report *report;
	unsigned int size;
	list_for_each_entry(report, &hid->report_enum[type].report_list, list) {
		size = ((report->size - 1) >> 3) + 1 + hid->report_enum[type].numbered;
		if (*max < size)
			*max = size;
	}
}

static int hid_alloc_buffers(struct usb_device *dev, struct hid_device *hid)
{
	struct usbhid_device *usbhid = hid->driver_data;
	usbhid->inbuf = usb_alloc_coherent(dev, usbhid->bufsize, GFP_KERNEL, &usbhid->inbuf_dma);
	usbhid->cr = kzalloc(sizeof(*usbhid->cr), GFP_KERNEL);

	return 0;
}


static void hid_free_buffers(struct usb_device *dev, struct hid_device *hid)
{
	struct usbhid_device *usbhid = hid->driver_data;
	usb_free_coherent(dev, usbhid->bufsize, usbhid->inbuf, usbhid->inbuf_dma);
	kfree(usbhid->cr);
}

static int usbhid_parse(struct hid_device *hid)
{
	struct usb_interface *intf = to_usb_interface(hid->dev.parent);
	struct usb_host_interface *interface = intf->cur_altsetting;
	struct usb_device *dev = interface_to_usbdev (intf);
	struct hid_descriptor *hdesc;
	unsigned int rsize = 0;
	char *rdesc;
	int ifnum = interface->desc.bInterfaceNumber;

	usb_get_extra_descriptor(interface, (USB_TYPE_CLASS | 0x01), &hdesc);

	rsize = le16_to_cpu(hdesc->desc[0].wDescriptorLength);
	rdesc = kmalloc(rsize, GFP_KERNEL);

	usb_control_msg(dev, usb_sndctrlpipe(dev, 0), 0x0A, USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
		0, ifnum, NULL, 0, USB_CTRL_SET_TIMEOUT);

	memset(rdesc, 0, rsize);
	usb_control_msg(dev, usb_rcvctrlpipe(dev, 0), USB_REQ_GET_DESCRIPTOR, USB_RECIP_INTERFACE | USB_DIR_IN, 
		((USB_TYPE_CLASS | 0x02) << 8), ifnum, rdesc, rsize, USB_CTRL_GET_TIMEOUT);

	hid_parse_report(hid, rdesc, rsize);
	kfree(rdesc);

	return 0;
}

static int usbhid_start(struct hid_device *hid)
{
	struct usb_interface *intf = to_usb_interface(hid->dev.parent);
	struct usb_host_interface *interface = intf->cur_altsetting;
	struct usb_device *dev = interface_to_usbdev(intf);
	struct usbhid_device *usbhid = hid->driver_data;
	struct usb_endpoint_descriptor *endpoint;
	int pipe;
	int interval;
	unsigned int insize = 0;

	// clear_bit(7, &usbhid->iofl);
	usbhid->bufsize = 64;
	hid_find_max_report(hid, 0, &insize);
	hid_alloc_buffers(dev, hid);
	endpoint = &interface->endpoint[0].desc;
	interval = endpoint->bInterval;
	usbhid->urbin = usb_alloc_urb(0, GFP_KERNEL);
	pipe = usb_rcvintpipe(dev, endpoint->bEndpointAddress);

	usb_fill_int_urb(usbhid->urbin, dev, pipe, usbhid->inbuf, insize, hid_irq_in, hid, interval);

	usbhid->urbctrl = usb_alloc_urb(0, GFP_KERNEL);
	usb_fill_control_urb(usbhid->urbctrl, dev, 0, (void *) usbhid->cr, usbhid->ctrlbuf, 1, hid_ctrl, hid);
	set_bit(8, &usbhid->iofl);

	return 0;
}

static void usbhid_stop(struct hid_device *hid)
{
	struct usbhid_device *usbhid = hid->driver_data;

	clear_bit(8, &usbhid->iofl);
	set_bit(7, &usbhid->iofl);
	usb_kill_urb(usbhid->urbin);
	usb_kill_urb(usbhid->urbctrl);

	usb_free_urb(usbhid->urbin);
	usb_free_urb(usbhid->urbctrl);
	// usbhid->urbin   = NULL; /* don't mess up next start */
	// usbhid->urbctrl = NULL;

	hid_free_buffers(hid_to_usb_dev(hid), hid); // Probably causes leaks
}

static int usbhid_raw_request(struct hid_device *hid, unsigned char reportnum, __u8 *buf, size_t len, unsigned char rtype, int reqtype)
{
	return -EIO;
}



static struct hid_ll_driver usb_hid_driver = {
	.parse = usbhid_parse,
	.start = usbhid_start,
	.stop = usbhid_stop,
	.open = usbhid_open,
	.close = usbhid_close,
	.raw_request = usbhid_raw_request,
};


static int usbhid_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_host_interface *interface = intf->cur_altsetting;
	struct usbhid_device *usbhid;
	struct hid_device *hid;

	usb_endpoint_is_int_in(&interface->endpoint[0].desc);

	hid = hid_allocate_device();

	usb_set_intfdata(intf, hid);
	hid->ll_driver = &usb_hid_driver;
	hid->ff_init = 0;
	hid->dev.parent = &intf->dev;
	hid->bus = 0x03;

	strlcpy(hid->name, "Logitech USB Receiver\0", sizeof(hid->name));
	usbhid = kzalloc(sizeof(*usbhid), GFP_KERNEL);

	hid->driver_data = usbhid;
	usbhid->hid = hid;
	usbhid->intf = intf;
	usbhid->ifnum = interface->desc.bInterfaceNumber;

	hid_add_device(hid);

	return 0;
}

static void usbhid_disconnect(struct usb_interface *intf)
{
	struct hid_device *hid = usb_get_intfdata(intf);
	struct usbhid_device *usbhid;

	usbhid = hid->driver_data;
	hid_destroy_device(hid);
	kfree(usbhid);
}


static const struct usb_device_id hid_usb_ids[] = {
	{.match_flags = USB_DEVICE_ID_MATCH_INT_CLASS, .bInterfaceClass = 3},
	{}						/* Terminating entry */
};

static struct usb_driver hid_driver = {
	.name =		"usbhid",
	.probe =	usbhid_probe,
	.disconnect =	usbhid_disconnect,
	.id_table =	hid_usb_ids,
};

static int __init hid_init(void)
{
	usb_register(&hid_driver);
	printk(KERN_INFO KBUILD_MODNAME ": " "USB HID core driver" "\n");
	return 0;
}

static void __exit hid_exit(void)
{
	usb_deregister(&hid_driver);
}

module_init(hid_init);
module_exit(hid_exit);

MODULE_DEVICE_TABLE (usb, hid_usb_ids);
MODULE_AUTHOR("Blayson! Muahaha!");
MODULE_DESCRIPTION("USB HID core driver");
MODULE_LICENSE("GPL");
