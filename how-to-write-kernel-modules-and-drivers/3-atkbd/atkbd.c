/* My AT and PS/2 keyboard driver */

/* I've cut this driver down to about 1/3 its original size.
 * I probably fucked a few things up, but if so, I can't tell.
 * It still seems to work fine. ~Me
 */

/*
 * This driver can handle standard AT keyboards and PS/2 keyboards in
 * Translated and Raw Set 2 and Set 3, as well as AT keyboards on dumb
 * input-only controllers and AT keyboards connected over a one way RS232
 * converter.
 */

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/serio.h>
#include <linux/workqueue.h>
#include <linux/libps2.h>
#include <linux/mutex.h>
#include <linux/dmi.h>
#include <linux/kernel.h>   /* Added this for printk() */

#define DRIVER_DESC	"AT and PS/2 keyboard driver"

MODULE_AUTHOR("Jason Mothafuckin Wilkes");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");


/* Scancode to keycode tables. These are just the default setting, and are loadable via a userland utility. */
#define ATKBD_KEYMAP_SIZE	512
static const unsigned short atkbd_set2_keycode[ATKBD_KEYMAP_SIZE] = {
	  0, 67, 65, 63, 61, 59, 60, 88,  0, 68, 66, 64, 62, 15, 41,117,
	  0, 56, 42, 93, 29, 16,  2,  0,  0,  0, 44, 31, 30, 17,  3,  0,
	  0, 46, 45, 32, 18,  5,  4, 95,  0, 57, 47, 33, 20, 19,  6,183,
	  0, 49, 48, 35, 34, 21,  7,184,  0,  0, 50, 36, 22,  8,  9,185,
	  0, 51, 37, 23, 24, 11, 10,  0,  0, 52, 53, 38, 39, 25, 12,  0,
	  0, 89, 40,  0, 26, 13,  0,  0, 58, 54, 28, 27,  0, 43,  0, 85,
	  0, 86, 91, 90, 92,  0, 14, 94,  0, 79,124, 75, 71,121,  0,  0,
	 82, 83, 80, 76, 77, 72,  1, 69, 87, 78, 81, 74, 55, 73, 70, 99,

	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	217,100,255,  0, 97,165,  0,  0,156,  0,  0,  0,  0,  0,  0,125,
	173,114,  0,113,  0,  0,  0,126,128,  0,  0,140,  0,  0,  0,127,
	159,  0,115,  0,164,  0,  0,116,158,  0,172,166,  0,  0,  0,142,
	157,  0,  0,  0,  0,  0,  0,  0,155,  0, 98,  0,  0,163,  0,  0,
	226,  0,  0,  0,  0,  0,  0,  0,  0,255, 96,  0,  0,  0,143,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,107,  0,105,102,  0,  0,112,
	110,111,108,112,106,103,  0,119,  0,118,109,  0, 99,104,119,  0,

	  0,  0,  0, 65, 99,
};

static const unsigned short atkbd_unxlate_table[128] = {
          0,118, 22, 30, 38, 37, 46, 54, 61, 62, 70, 69, 78, 85,102, 13,
         21, 29, 36, 45, 44, 53, 60, 67, 68, 77, 84, 91, 90, 20, 28, 27,
         35, 43, 52, 51, 59, 66, 75, 76, 82, 14, 18, 93, 26, 34, 33, 42,
         50, 49, 58, 65, 73, 74, 89,124, 17, 41, 88,  5,  6,  4, 12,  3,
         11,  2, 10,  1,  9,119,126,108,117,125,123,107,115,116,121,105,
        114,122,112,113,127, 96, 97,120,  7, 15, 23, 31, 39, 47, 55, 63,
         71, 79, 86, 94,  8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 87,111,
         19, 25, 57, 81, 83, 92, 95, 98, 99,100,101,103,104,106,109,110
};

#define ATKBD_CMD_ENABLE	0x00f4
#define ATKBD_CMD_GETID		0x02f2
#define ATKBD_CMD_SETLEDS	0x20eb
#define ATKBD_CMD_SETREP	0x10f3 
#define ATKBD_CMD_RESET_DEF	0x00f6	/* Reset to defaults */


#define ATKBD_RET_ACK		0xfa
#define ATKBD_RET_NAK		0xfe
#define ATKBD_RET_BAT		0xaa
#define ATKBD_RET_EMUL0		0xe0
#define ATKBD_RET_EMUL1		0xe1
#define ATKBD_RET_RELEASE	0xf0
#define ATKBD_RET_HANJA		0xf1
#define ATKBD_RET_HANGEUL	0xf2
#define ATKBD_RET_ERR		0xff

#define ATKBD_KEY_NULL		255
#define ATKBD_SPECIAL		0xfff8


/* The atkbd control structure */
struct atkbd {

	struct ps2dev ps2dev;
	struct input_dev *dev;

	/* Written only during init */
	char name[64];
	char phys[32];

	unsigned short id;
	unsigned short keycode[ATKBD_KEYMAP_SIZE];
	DECLARE_BITMAP(force_release_mask, ATKBD_KEYMAP_SIZE);
	unsigned char set;
	bool translated;
	bool extra;
	bool write;
	bool softrepeat;
	bool softraw;
	bool scroll;
	bool enabled;

	/* Accessed only from interrupt */
	unsigned char emul;
	bool resend;
	bool release;
	unsigned long xl_bit;
	unsigned int last;
	unsigned long time;
	unsigned long err_count;

	struct delayed_work event_work;
	unsigned long event_jiffies;
	unsigned long event_mask;

	/* Serializes reconnect(), attr->set() and event work */
	struct mutex mutex;
};

/* System-specific keymap fixup routine */
static ssize_t atkbd_attr_show_helper(struct device *dev, char *buf, ssize_t (*handler)(struct atkbd *, char *));
static ssize_t atkbd_attr_set_helper(struct device *dev, const char *buf, size_t count, ssize_t (*handler)(struct atkbd *, const char *, size_t));

#define ATKBD_DEFINE_ATTR(_name)						\
static ssize_t atkbd_show_##_name(struct atkbd *, char *);			\
static ssize_t atkbd_set_##_name(struct atkbd *, const char *, size_t);		\
static ssize_t atkbd_do_show_##_name(struct device *d,				\
				struct device_attribute *attr, char *b)		\
{										\
	return atkbd_attr_show_helper(d, b, atkbd_show_##_name);		\
}										\
static ssize_t atkbd_do_set_##_name(struct device *d,				\
			struct device_attribute *attr, const char *b, size_t s)	\
{										\
	return atkbd_attr_set_helper(d, b, s, atkbd_set_##_name);		\
}										\
static struct device_attribute atkbd_attr_##_name =				\
	__ATTR(_name, S_IWUSR | S_IRUGO, atkbd_do_show_##_name, atkbd_do_set_##_name);


ATKBD_DEFINE_ATTR(extra);
ATKBD_DEFINE_ATTR(force_release);
ATKBD_DEFINE_ATTR(scroll);
ATKBD_DEFINE_ATTR(set);
ATKBD_DEFINE_ATTR(softrepeat);
ATKBD_DEFINE_ATTR(softraw);

#define ATKBD_DEFINE_RO_ATTR(_name)						\
static ssize_t atkbd_show_##_name(struct atkbd *, char *);			\
static ssize_t atkbd_do_show_##_name(struct device *d,				\
				struct device_attribute *attr, char *b)		\
{										\
	return atkbd_attr_show_helper(d, b, atkbd_show_##_name);		\
}										\
static struct device_attribute atkbd_attr_##_name =				\
	__ATTR(_name, S_IRUGO, atkbd_do_show_##_name, NULL);

ATKBD_DEFINE_RO_ATTR(err_count);

static struct attribute *atkbd_attributes[] = {
	&atkbd_attr_extra.attr,
	&atkbd_attr_force_release.attr,
	&atkbd_attr_scroll.attr,
	&atkbd_attr_set.attr,
	&atkbd_attr_softrepeat.attr,
	&atkbd_attr_softraw.attr,
	&atkbd_attr_err_count.attr,
	NULL
};

static struct attribute_group atkbd_attribute_group = {
	.attrs	= atkbd_attributes,
};

static const unsigned int xl_table[] = {
	ATKBD_RET_BAT, ATKBD_RET_ERR, ATKBD_RET_ACK,
	ATKBD_RET_NAK, ATKBD_RET_HANJA, ATKBD_RET_HANGEUL,
};

/* Checks if we should mangle the scancode to extract 'release' bit in translated mode. */
static bool atkbd_need_xlate(unsigned long xl_bit, unsigned char code)
{
	int i;
	printk("[*] In atkbd_need_xlate\n");

	if (code == ATKBD_RET_EMUL0 || code == ATKBD_RET_EMUL1) {
		return false;
        }

	for (i = 0; i < ARRAY_SIZE(xl_table); i++)
		if (code == xl_table[i])
			return test_bit(i, &xl_bit);

	return true;
}


/* atkbd_interrupt(). Here takes place processing of data received from the keyboard into events. */
static irqreturn_t atkbd_interrupt(struct serio *serio, unsigned char data, unsigned int flags)
{
	struct atkbd *atkbd = serio_get_drvdata(serio);
	struct input_dev *dev = atkbd->dev;
	unsigned int code = data;
	int scroll = 0, hscroll = 0, click = -1;
	int value;
	unsigned short keycode;
        int i;

	/* NOTE TO SELF:
	 * =============
	 * It looks like the "data" is the keycode. That is, data is 
	 * 0x01 for ESC, 0x02 for 1, etc., and the same value + 0x80 when that key is released.
	 */
	printk("[*] In atkbd_interrupt: data == 0x%02x, flags == %d\n", data, flags);

	if (unlikely(atkbd->ps2dev.flags & PS2_FLAG_ACK))
		if  (ps2_handle_ack(&atkbd->ps2dev, data))
			goto out;

	if (unlikely(atkbd->ps2dev.flags & PS2_FLAG_CMD))
		if  (ps2_handle_response(&atkbd->ps2dev, data))
			goto out;

	if (!atkbd->enabled)
		goto out;

	input_event(dev, EV_MSC, MSC_RAW, code);

	if (atkbd->translated) {

		if (atkbd->emul || atkbd_need_xlate(atkbd->xl_bit, code)) {
			atkbd->release = code >> 7;
			code &= 0x7f;
		}

                /* Variable i is local to this block */
		if (!atkbd->emul) {
                        /* Calculates new value of xl_bit so the driver can distinguish between make/break 
                         * pair of scancodes for select keys and PS/2 protocol responses. */
	                for (i = 0; i < ARRAY_SIZE(xl_table); i++) {
		                if (!((code ^ xl_table[i]) & 0x7f)) {
			                if (code & 0x80)
				                __clear_bit(i, &atkbd->xl_bit);
			                else
				                __set_bit(i, &atkbd->xl_bit);
			                break;
		                }
	                }
                }
	}

	switch (code) {
	case ATKBD_RET_BAT:
		atkbd->enabled = false;
		serio_reconnect(atkbd->ps2dev.serio);
		goto out;
	case ATKBD_RET_EMUL0:
		atkbd->emul = 1;
		goto out;
	}


        /* This block used to be atkbd_compat_scancode() */
        /* Encode the scancode, 0xe0 prefix, and high bit into a single integer, keeping kernel 2.4 compatibility for set 2 */
	code = (code & 0x7f) | ((code & 0x80) << 1);
	if (atkbd->emul == 1)
		code |= 0x80;


	if (atkbd->emul && --atkbd->emul)
		goto out;

	keycode = atkbd->keycode[code];

	if (keycode != ATKBD_KEY_NULL)
		input_event(dev, EV_MSC, MSC_SCAN, code);

	if (atkbd->release) {
		value = 0;
		atkbd->last = 0;
	} else {
		value = 1;
		atkbd->last = code;
		atkbd->time = jiffies + msecs_to_jiffies(dev->rep[REP_DELAY]) / 2;
	}

	input_event(dev, EV_KEY, keycode, value);
	input_sync(dev);

	if (value && test_bit(code, atkbd->force_release_mask)) {
		input_report_key(dev, keycode, 0);
		input_sync(dev);
	}

	if (atkbd->scroll) {
		if (click != -1)
			input_report_key(dev, BTN_MIDDLE, click);
		input_report_rel(dev, REL_WHEEL, atkbd->release ? -scroll : scroll);
		input_report_rel(dev, REL_HWHEEL, hscroll);
		input_sync(dev);
	}

	atkbd->release = false;
out:
	return IRQ_HANDLED;
}


/*
 * atkbd_connect() is called when the serio module finds an interface
 * that isn't handled yet by an appropriate device driver. We check if
 * there is an AT keyboard out there and if yes, we register ourselves
 * to the input module.
 */

static int atkbd_connect(struct serio *serio, struct serio_driver *drv)
{
	struct atkbd *atkbd;
	struct input_dev *dev;
	int err = -ENOMEM;

	struct ps2dev *ps2dev;
	unsigned char param[2];

	unsigned int scancode;
	int i;

	struct input_dev *input_dev;
        int j;

	printk("[*] In atkbd_connect\n");

	atkbd = kzalloc(sizeof(struct atkbd), GFP_KERNEL);
	dev = input_allocate_device();

	atkbd->dev = dev;
	ps2_init(&atkbd->ps2dev, serio);
	mutex_init(&atkbd->mutex);

	switch (serio->id.type) {

	case SERIO_8042_XL:
		atkbd->translated = true;
		/* Fall through */

	case SERIO_8042:
		if (serio->write)
			atkbd->write = true;
		break;
	}

	atkbd->softraw = true;
	atkbd->softrepeat = 0;
	atkbd->scroll = 0;

	serio_set_drvdata(serio, atkbd);
	serio_open(serio, drv);


        /*********************/
        /* BEGIN ATKBD PROBE */
        /*********************/
        /* This was atkbd_probe(atkbd); */
        /* The variables struct ps2dev *ps2dev and param are local to this block */
        /* FROM THE ORIGINAL FUNCTION:
         * Then we check the keyboard ID. We should get 0xab83 under normal conditions.
         * Some keyboards report different values, but the first byte is always 0xab or
         * 0xac. Some old AT keyboards don't report anything. If a mouse is connected, this
         * should make sure we don't try to set the LEDs on it. */
        ps2dev = &atkbd->ps2dev;

	param[0] = param[1] = 0xa5;	/* initialize with invalid values */
	if (ps2_command(ps2dev, param, ATKBD_CMD_GETID)) {
		param[0] = 0;
		if (ps2_command(ps2dev, param, ATKBD_CMD_SETLEDS)) {
			err = -ENODEV;
                        goto fail;
                }
		atkbd->id = 0xabba;
	} else {
	        atkbd->id = (param[0] << 8) | param[1];
	        //atkbd_deactivate(atkbd);
        }
        /*******************/
        /* END ATKBD PROBE */
        /*******************/

	atkbd->set = 2;

        /***************************/
        /* BEGIN SET KEYCODE TABLE */
        /***************************/
        /* This was atkbd_set_keycode_table(atkbd); */
        /* The variables scancode and i are local to this block */
	printk("[*] In atkbd_set_keycode_table\n");

	memset(atkbd->keycode, 0, sizeof(atkbd->keycode));
	bitmap_zero(atkbd->force_release_mask, ATKBD_KEYMAP_SIZE);

	// Since (atkbd->translated)
	for (i = 0; i < 128; i++) {
		scancode = atkbd_unxlate_table[i];
		atkbd->keycode[i] = atkbd_set2_keycode[scancode];
		atkbd->keycode[i | 0x80] = atkbd_set2_keycode[scancode | 0x80];
	}

        /*************************/
        /* END SET KEYCODE TABLE */
        /*************************/

        /**************************/
        /* BEGIN SET DEVICE ATTRS */
        /**************************/
        /* This was atkbd_set_device_attrs(atkbd); */
        /* The variables input_dev and j are local to this block */
        input_dev = atkbd->dev;
	printk("[*] In atkbd_set_device_attrs\n");

	snprintf(atkbd->name, sizeof(atkbd->name), "AT %s Set %d keyboard", atkbd->translated ? "Translated" : "Raw", atkbd->set);
	snprintf(atkbd->phys, sizeof(atkbd->phys), "%s/input0", atkbd->ps2dev.serio->phys);

	input_dev->name = atkbd->name;
	input_dev->phys = atkbd->phys;
	input_dev->id.bustype = BUS_I8042;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = atkbd->translated ? 1 : atkbd->set;
	input_dev->id.version = atkbd->id;
	// input_dev->event = atkbd_event;
	input_dev->dev.parent = &atkbd->ps2dev.serio->dev;
	input_set_drvdata(input_dev, atkbd);
	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP) | BIT_MASK(EV_MSC);
	input_dev->evbit[0] |= BIT_MASK(EV_LED);
	input_dev->ledbit[0] = BIT_MASK(LED_NUML) | BIT_MASK(LED_CAPSL) | BIT_MASK(LED_SCROLLL);
	input_dev->rep[REP_DELAY] = 250;
	input_dev->rep[REP_PERIOD] = 33;
	input_dev->mscbit[0] = atkbd->softraw ? BIT_MASK(MSC_SCAN) : BIT_MASK(MSC_RAW) | BIT_MASK(MSC_SCAN);
	input_dev->keycode = atkbd->keycode;
	input_dev->keycodesize = sizeof(unsigned short);
	input_dev->keycodemax = ARRAY_SIZE(atkbd_set2_keycode);

	for (j = 0; j < ATKBD_KEYMAP_SIZE; j++) {
		if (atkbd->keycode[j] != KEY_RESERVED &&
		    atkbd->keycode[j] != ATKBD_KEY_NULL &&
		    atkbd->keycode[j] < ATKBD_SPECIAL) {
			__set_bit(atkbd->keycode[j], input_dev->keybit);
		}
	}
        /************************/
        /* END SET DEVICE ATTRS */
        /************************/


	sysfs_create_group(&serio->dev.kobj, &atkbd_attribute_group);

	atkbd->enabled = true;
	if (serio->write) {
	        if (ps2_command(ps2dev, NULL, ATKBD_CMD_ENABLE))
		        dev_err(&ps2dev->serio->dev, "Failed to enable keyboard on %s\n", ps2dev->serio->phys);
        }

	input_register_device(atkbd->dev);

	return 0;

fail:	serio_close(serio);
        serio_set_drvdata(serio, NULL);
        input_free_device(dev);
	kfree(atkbd);
	return err;
}

/* atkbd_disconnect() closes and frees. */
static void atkbd_disconnect(struct serio *serio)
{
	struct atkbd *atkbd = serio_get_drvdata(serio);

	printk("[*] In atkbd_disconnect\n");
	sysfs_remove_group(&serio->dev.kobj, &atkbd_attribute_group);
	atkbd->enabled = false;
	input_unregister_device(atkbd->dev);

	/* Make sure we don't have a command in flight. */
	cancel_delayed_work_sync(&atkbd->event_work);
	serio_close(serio);
	serio_set_drvdata(serio, NULL);
	kfree(atkbd);
}

static struct serio_device_id atkbd_serio_ids[] = {
	{
		.type	= SERIO_8042,
		.proto	= SERIO_ANY,
		.id	= SERIO_ANY,
		.extra	= SERIO_ANY,
	},
	{
		.type	= SERIO_8042_XL,
		.proto	= SERIO_ANY,
		.id	= SERIO_ANY,
		.extra	= SERIO_ANY,
	},
	{
		.type	= SERIO_RS232,
		.proto	= SERIO_PS2SER,
		.id	= SERIO_ANY,
		.extra	= SERIO_ANY,
	},
	{ 0 }
};

static struct serio_driver atkbd_drv = {
	.driver		= {
		.name	= "atkbd",
	},
	.description	= DRIVER_DESC,
	.id_table	= atkbd_serio_ids,
	.interrupt	= atkbd_interrupt,
	.connect	= atkbd_connect,
	.disconnect	= atkbd_disconnect,
};

static ssize_t atkbd_attr_show_helper(struct device *dev, char *buf, ssize_t (*handler)(struct atkbd *, char *)){return 0;}
static ssize_t atkbd_attr_set_helper(struct device *d, const char *b, size_t c, ssize_t (*handler)(struct atkbd *, const char *, size_t)){return 0;}
static ssize_t atkbd_show_extra(struct atkbd *atkbd, char *buf){return 0;}
static ssize_t atkbd_set_extra(struct atkbd *atkbd, const char *buf, size_t count){return 0;}
static ssize_t atkbd_show_force_release(struct atkbd *atkbd, char *buf){return 0;}
static ssize_t atkbd_set_force_release(struct atkbd *atkbd, const char *buf, size_t count){return 0;}
static ssize_t atkbd_show_scroll(struct atkbd *atkbd, char *buf){return 0;}
static ssize_t atkbd_set_scroll(struct atkbd *atkbd, const char *buf, size_t count){return 0;}
static ssize_t atkbd_show_set(struct atkbd *atkbd, char *buf){return 0;}
static ssize_t atkbd_set_set(struct atkbd *atkbd, const char *buf, size_t count){return 0;}
static ssize_t atkbd_show_softrepeat(struct atkbd *atkbd, char *buf){return 0;}
static ssize_t atkbd_set_softrepeat(struct atkbd *atkbd, const char *buf, size_t count){return 0;}
static ssize_t atkbd_show_softraw(struct atkbd *atkbd, char *buf){return 0;}
static ssize_t atkbd_set_softraw(struct atkbd *atkbd, const char *buf, size_t count){return 0;}
static ssize_t atkbd_show_err_count(struct atkbd *atkbd, char *buf){return 0;}

static int __init atkbd_init(void)
{
	printk("[*] In atkbd_init\n");
	return serio_register_driver(&atkbd_drv);
}

static void __exit atkbd_exit(void)
{
	printk("[*] In atkbd_exit\n");
	serio_unregister_driver(&atkbd_drv);
}

module_init(atkbd_init);
module_exit(atkbd_exit);

