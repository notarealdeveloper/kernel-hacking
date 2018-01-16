/* The function hid_irq_in(struct urb *urb) is where the magic is.
 * For more ideas, look in include/linux/usb.h for attributes
 * of the "struct urb" structure, since that's how USB transfers data. */

// printk("urb->transfer_buffer_length = %d\n", urb->transfer_buffer_length); 	/* Always returns 8 */
// printk("urb->transfer_buffer = %p\n", urb->transfer_buffer);			/* This is the thing to fuck with! */

#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>

#define enter() printk(KERN_DEBUG "[+] %s: entering\n", __func__)
#define leave() printk(KERN_DEBUG "[-] %s: leaving\n", __func__)
/***************/
/* Begin hid.h */
/***************/

struct hid_field {
	__s32    *value;		/* last known value(s) */
	struct hid_report *report;	/* associated report */
	unsigned index;			/* index into report->field[] */
	struct hid_input *hidinput;	/* associated input structure */
};


struct hid_report {
	struct list_head list;
	unsigned id;					/* id of this report */
	unsigned type;					/* report type */
	void 	*fuckit_0[256];
	unsigned fuckit_1;
	unsigned size;					/* size of the report (bits) */
	struct hid_device *device;			/* associated device */
};


struct hid_report_enum {
	unsigned numbered;
	struct list_head report_list;
	//struct hid_report *report_id_hash[256];
	void *fuckit_0[256];
};


struct hid_input {
	struct list_head list;
	struct hid_report *report;
	struct input_dev *input;
};

/* Something in usbcore is using this. Can't make normal assumptions
 * Update: The weird behavior was because the order of members in this structure matters! 
 * Something in usbcore or elsewhere must be *parsing* this shit! */
struct hid_device {					/* device report descriptor */
	char 	fuckit_1[16];
	u8 	*rdesc;
	unsigned rsize;
	char 	fuckit_2[24];
	u16 	bus;					/* BUS ID */
	char 	fuckit_3[22];
	struct 	hid_report_enum report_enum[3];
	char 	fuckit_4[80];
	struct 	device dev;				/* device */
	char 	fuckit_5[8];
	struct 	hid_ll_driver *ll_driver;
	char 	fuckit_6[56];
	char 	name[128];				/* Device name */
	char 	phys[64];
	void 	*driver_data;
};


struct hid_descriptor {
	u8  fuckit[7];
	u16 len;
} __attribute__((packed));


struct hid_ll_driver {
	int  (*start)(struct hid_device *hdev);
	void (*stop) (struct hid_device *hdev);
	int  (*open) (struct hid_device *hdev);
	void (*close)(struct hid_device *hdev);
	void *fuckit_power;
	int  (*parse)(struct hid_device *hdev);
	void *fuckit_request;
	void *fuckit_wait;
	int  (*raw_request) (struct hid_device *hdev, unsigned char reportnum, __u8 *buf, size_t len, unsigned char rtype, int reqtype);
	void *fuckit_idle;
};

/* HID core API */
int  hid_add_device(struct hid_device *);
void hid_destroy_device(struct hid_device *);


/****************/
/* End of hid.h */
/****************/



int hid_input_report(struct hid_device *, int type, u8 *, int, int);
struct hid_device *hid_allocate_device(void);
int hid_parse_report(struct hid_device *hid, __u8 *start, unsigned size);     

/* USB-specific HID struct, to be pointed to from struct hid_device->driver_data */
struct usbhid_device {
	struct hid_device *hid;						/* pointer to corresponding HID dev */

	struct usb_interface *intf;                                     /* USB interface */
	int ifnum;                                                      /* USB interface number */

	struct urb *urbin;                                              /* Input URB */
	char *inbuf;                                                    /* Input buffer */
	dma_addr_t inbuf_dma;                                           /* Input buffer dma */

	struct urb *urbctrl;                                            /* Control URB */
	struct usb_ctrlrequest *cr;                                     /* Control request struct */
	char *ctrlbuf;                                                  /* Control buffer */
};

#define usbhid_bufsize	64
#define	hid_to_usb_dev(hid_dev) 	container_of(hid_dev->dev.parent->parent, struct usb_device, dev)

/* Input interrupt completion handler. */
static void hid_irq_in(struct urb *urb)
{
	hid_input_report(urb->context, 0, urb->transfer_buffer, urb->actual_length, 1);
	usb_submit_urb(urb, GFP_ATOMIC);
}

int usbhid_open(struct hid_device *hid)
{
	struct usbhid_device *usbhid = hid->driver_data;
	enter();
	usb_submit_urb(usbhid->urbin, GFP_ATOMIC);
	usb_autopm_put_interface(usbhid->intf);
	leave();
	return 0;
}

void usbhid_close(struct hid_device *hid)
{
	struct usbhid_device *usbhid = hid->driver_data;
	enter();
	usb_kill_urb(usbhid->urbin);
	usbhid->intf->needs_remote_wakeup = 0;
	leave();
}

static int usbhid_parse(struct hid_device *hid)
{
	struct usb_interface *intf = to_usb_interface(hid->dev.parent);
	struct usb_host_interface *interface = intf->cur_altsetting;
	struct usb_device *dev = interface_to_usbdev (intf);
	struct hid_descriptor *hdesc;
	unsigned int rsize = 0;
	char *rdesc;
	int ifnum;

	enter();
	ifnum = interface->desc.bInterfaceNumber;
	usb_get_extra_descriptor(interface, (USB_TYPE_CLASS | 0x01), &hdesc);
	rsize = hdesc->len;
	rdesc = kzalloc(rsize, GFP_KERNEL);

	/* Final argument == timeout == 5000 msec */
	usb_control_msg(dev, usb_sndctrlpipe(dev, 0), 0x0A, USB_TYPE_CLASS | USB_RECIP_INTERFACE, 0, ifnum, NULL, 0, 5000);
	usb_control_msg(dev, usb_rcvctrlpipe(dev, 0), USB_REQ_GET_DESCRIPTOR, USB_RECIP_INTERFACE | USB_DIR_IN, 
		((USB_TYPE_CLASS | 0x02) << 8), ifnum, rdesc, rsize, 5000);

	hid_parse_report(hid, rdesc, rsize);
	kfree(rdesc);
	leave();
	return 0;
}

static int usbhid_start(struct hid_device *hid)
{
	struct usb_interface 		*intf 		= to_usb_interface(hid->dev.parent);
	struct usb_host_interface 	*interface 	= intf->cur_altsetting;
	struct usb_device 		*dev 		= interface_to_usbdev(intf);
	struct usbhid_device 		*usbhid 	= hid->driver_data;
	struct usb_endpoint_descriptor 	*endpoint;
	int pipe;
	int interval;

	struct hid_report *report;
	unsigned int size;
	unsigned int maxsize = 0;

	enter();
	// printk("sizeof(struct list_head) == %lu\n", sizeof(struct list_head));
	// printk("&(hid->inputs)       == %p\n", &(hid->inputs));
	/* Was hid_find_max_report */
	list_for_each_entry(report, &hid->report_enum[0].report_list, list) {
		size = ((report->size - 1) >> 3) + 1 + hid->report_enum[0].numbered;
		if (maxsize < size)
			maxsize = size;
	}

	/* Was hid_alloc_buffers */
	usbhid->inbuf = usb_alloc_coherent(dev, usbhid_bufsize, GFP_KERNEL, &usbhid->inbuf_dma);
	usbhid->cr = kzalloc(sizeof(*usbhid->cr), GFP_KERNEL);

	endpoint = &interface->endpoint[0].desc;
	interval = endpoint->bInterval;
	usbhid->urbin = usb_alloc_urb(0, GFP_KERNEL);
	// Next line does this: pipe = usb_rcvintpipe(dev, endpoint->bEndpointAddress);
	printk("endpoint->bEndpointAddress == %u\n", endpoint->bEndpointAddress);
	pipe = ((1 << 30) | (dev->devnum << 8) | (endpoint->bEndpointAddress << 15) | 0x80);

	usb_fill_int_urb(usbhid->urbin, dev, pipe, usbhid->inbuf, maxsize, hid_irq_in, hid, interval);
	usbhid->urbctrl = usb_alloc_urb(0, GFP_KERNEL);
	leave();
	return 0;
}

static void usbhid_stop(struct hid_device *hid)
{
	struct usbhid_device *usbhid = hid->driver_data;

	enter();
	usb_kill_urb(usbhid->urbin);
	usb_kill_urb(usbhid->urbctrl);
	usb_free_urb(usbhid->urbin);
	usb_free_urb(usbhid->urbctrl);
	usb_free_coherent(hid_to_usb_dev(hid), usbhid_bufsize, usbhid->inbuf, usbhid->inbuf_dma);
	kfree(usbhid->cr);
	leave();
}

static int usbhid_raw_request(struct hid_device *hid, u8 reportnum, __u8 *buf, size_t len, u8 rtype, int reqtype)
{
	printk("GODDAMMIT USB!!!\n");
	return -EIO;
}


static struct hid_ll_driver usb_hid_driver = {
	.start 		= usbhid_start,
	.stop  		= usbhid_stop,
	.open  		= usbhid_open,
	.close 		= usbhid_close,
	.parse 		= usbhid_parse,
	.raw_request 	= usbhid_raw_request,
};


static int usbhid_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_host_interface *interface = intf->cur_altsetting;
	struct usbhid_device *usbhid;
	struct hid_device *hid;

	enter();
	usb_endpoint_is_int_in(&interface->endpoint[0].desc);

	hid = hid_allocate_device();

	usb_set_intfdata(intf, hid);
	hid->ll_driver = &usb_hid_driver;
	hid->dev.parent = &intf->dev;
	hid->bus = 0x03;
	strncpy(hid->name, "The honorable mouse of Jason Wilkes", sizeof(hid->name));

	usbhid = kzalloc(sizeof(*usbhid), GFP_KERNEL);
	hid->driver_data = usbhid;
	usbhid->hid = hid;
	usbhid->intf = intf;

	hid_add_device(hid);
	leave();
	return 0;
}

static void usbhid_disconnect(struct usb_interface *intf)
{
	struct hid_device *hid = usb_get_intfdata(intf);
	struct usbhid_device *usbhid;
	enter();
	usbhid = hid->driver_data;
	hid_destroy_device(hid);
	kfree(usbhid);
	leave();
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

static int __init usbhid_init(void)
{
	enter();
	usb_register(&hid_driver);
	leave();
	return 0;
}

static void __exit usbhid_exit(void)
{
	enter();
	usb_deregister(&hid_driver);
	leave();
}

module_init(usbhid_init);
module_exit(usbhid_exit);

MODULE_DEVICE_TABLE (usb, hid_usb_ids);
MODULE_AUTHOR("你的媽, aka the woman who is and always has been, so fat.");
MODULE_LICENSE("GPL");
