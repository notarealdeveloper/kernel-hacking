/* The function hid_irq_in(struct urb *urb) is where the magic is.
 * For more ideas, look in include/linux/usb.h for attributes
 * of the "struct urb" structure, since that's how USB transfers data. */

// printk("urb->transfer_buffer_length = %d\n", urb->transfer_buffer_length); 	/* Always returns 8 */
// printk("urb->transfer_buffer = %p\n", urb->transfer_buffer);			/* This is the thing to fuck with! */

#include <linux/module.h>
#include <linux/usb.h>

/***************/
/* Begin hid.h */
/***************/

#include <linux/slab.h>

/* This is the collection stack. We climb up the stack to determine application and function of each field. */
struct hid_collection {
	unsigned type;
	unsigned usage;
	unsigned level;
};

struct hid_usage {
	unsigned  hid;			/* hid usage code */
	__u8      type;			/* input driver type */
};

struct hid_field {
	struct hid_usage *usage;	/* usage table for this function */
	__s32    *value;		/* last known value(s) */
	struct hid_report *report;	/* associated report */
	unsigned index;			/* index into report->field[] */
	struct hid_input *hidinput;	/* associated input structure */
};


struct hid_report {
	struct list_head list;
	unsigned id;					/* id of this report */
	unsigned type;					/* report type */
	struct hid_field *field[256];			/* fields of the report */
	unsigned maxfield;				/* maximum valid field index */
	unsigned size;					/* size of the report (bits) */
	struct hid_device *device;			/* associated device */
};


struct hid_report_enum {
	unsigned numbered;
	struct list_head report_list;
	struct hid_report *report_id_hash[256];
};


struct hid_input {
	struct list_head list;
	struct hid_report *report;
	struct input_dev *input;
};

enum hid_type {
	HID_TYPE_OTHER = 0,
	HID_TYPE_USBMOUSE,
	HID_TYPE_USBNONE
};

struct hid_device {							/* device report descriptor */
	__u8 *dev_rdesc;
	unsigned dev_rsize;
	__u8 *rdesc;
	unsigned rsize;
	struct hid_collection *collection;				/* List of HID collections */
	unsigned collection_size;					/* Number of allocated hid_collections */
	unsigned maxcollection;						/* Number of parsed collections */
	unsigned maxapplication;					/* Number of applications */
	__u16 bus;							/* BUS ID */
	__u16 group;							/* Report group */
	__u32 vendor;							/* Vendor ID */
	__u32 product;							/* Product ID */
	__u32 version;							/* HID version */
	enum hid_type type;						/* device type (mouse, kbd, ...) */
	unsigned country;						/* HID country */
	struct hid_report_enum report_enum[3];
	struct work_struct led_work;					/* delayed LED worker */

	struct semaphore driver_lock;					/* protects the current driver, except during input */
	struct semaphore driver_input_lock;				/* protects the current driver */
	struct device dev;						/* device */
	struct hid_driver *driver;
	struct hid_ll_driver *ll_driver;

	unsigned int status;						/* see STAT flags above */
	unsigned claimed;						/* Claimed by hidinput, hiddev? */
	unsigned quirks;						/* Various quirks the device can pull on us */
	bool io_started;						/* Protected by driver_lock. If IO has started */

	struct list_head inputs;					/* The list of inputs */
	void *hiddev;							/* The hiddev structure */
	void *hidraw;
	int minor;							/* Hiddev minor number */

	int open;							/* is the device open by anyone? */
	char name[128];							/* Device name */
	char phys[64];
	void *driver_data;

	/* temporary hid_ff handling (until moved to the drivers) */
	int (*ff_init)(struct hid_device *);

	/* hiddev event handler */
	int (*hiddev_connect)(struct hid_device *, unsigned int);
	void (*hiddev_disconnect)(struct hid_device *);
	void (*hiddev_hid_event) (struct hid_device *, struct hid_field *field, struct hid_usage *, __s32);
	void (*hiddev_report_event) (struct hid_device *, struct hid_report *);

	/* debugging support via debugfs */
	unsigned short debug;
	struct dentry *debug_dir;
	struct dentry *debug_rdesc;
	struct dentry *debug_events;
	struct list_head debug_list;
	spinlock_t  debug_list_lock;
	wait_queue_head_t debug_wait;
};

struct hid_class_descriptor {
	__u8  bDescriptorType;
	__le16 wDescriptorLength;
} __attribute__ ((packed));

struct hid_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;
	__le16 bcdHID;
	__u8  bCountryCode;
	__u8  bNumDescriptors;

	struct hid_class_descriptor desc[1];
} __attribute__ ((packed));


struct hid_report_id {
	__u32 report_type;
};
struct hid_usage_id {
	__u32 usage_hid;
	__u32 usage_type;
	__u32 usage_code;
};


struct hid_driver {
	char *name;
	const struct hid_device_id *id_table;
	struct list_head dyn_list;
	spinlock_t dyn_lock;
	int (*probe)(struct hid_device *dev, const struct hid_device_id *id);
	void (*remove)(struct hid_device *dev);
	const struct hid_report_id *report_table;
	int (*raw_event)(struct hid_device *hdev, struct hid_report *report, u8 *data, int size);
	const struct hid_usage_id *usage_table;
	int (*event)(struct hid_device *hdev, struct hid_field *field, struct hid_usage *usage, __s32 value);
	void (*report)(struct hid_device *hdev, struct hid_report *report);
	__u8 *(*report_fixup)(struct hid_device *hdev, __u8 *buf, unsigned int *size);
	int (*input_mapping)(struct hid_device *hdev, struct hid_input *hidinput, struct hid_field *field, struct hid_usage *usage, 
		unsigned long **bit, int *max);			
	int (*input_mapped)(struct hid_device *hdev, struct hid_input *hidinput, struct hid_field *field, struct hid_usage *usage, 
		unsigned long **bit, int *max);
	void (*input_configured)(struct hid_device *hdev,struct hid_input *hidinput);
	void (*feature_mapping)(struct hid_device *hdev,struct hid_field *field,struct hid_usage *usage);
	struct device_driver driver;
};


struct hid_ll_driver {
	int  (*start)(struct hid_device *hdev);
	void (*stop)(struct hid_device *hdev);
	int  (*open)(struct hid_device *hdev);
	void (*close)(struct hid_device *hdev);
	int  (*power)(struct hid_device *hdev, int level);
	int  (*parse)(struct hid_device *hdev);
	void (*request)(struct hid_device *hdev, struct hid_report *report, int reqtype);
	int  (*wait)(struct hid_device *hdev);
	int  (*raw_request) (struct hid_device *hdev, unsigned char reportnum, __u8 *buf, size_t len, unsigned char rtype, int reqtype);
	int  (*idle)(struct hid_device *hdev, int report, int idle, int reqtype);
};


/* HID core API */
extern int hid_debug;
extern bool hid_ignore(struct hid_device *);
extern int hid_add_device(struct hid_device *);
extern void hid_destroy_device(struct hid_device *);
extern int __must_check __hid_register_driver(struct hid_driver *, struct module *, const char *mod_name);
extern void hid_unregister_driver(struct hid_driver *);


#define module_hid_driver(__hid_driver) \
	module_driver(__hid_driver, hid_register_driver, hid_unregister_driver)

const struct hid_device_id *hid_match_id(struct hid_device *hdev, const struct hid_device_id *id);
s32 hid_snto32(__u32 value, unsigned n);


/****************/
/* End of hid.h */
/****************/



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

	printk("Before: rsize = %d\n", rsize);
	rsize = le16_to_cpu(hdesc->desc[0].wDescriptorLength);
	printk("After: rsize = %d\n", rsize);
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

	clear_bit(7, &usbhid->iofl);
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
