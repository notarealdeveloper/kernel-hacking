/* My AT and PS/2 keyboard driver (Was 45K -> now 7K)
 * I've cut this driver down to about 15% of its original size.
 * I probably fucked a few things up, but if so, I can't tell.
 * It still seems to work fine on my laptop. ~ Me */

#include <linux/module.h>
#include <linux/slab.h>             /* kzalloc, kfree */
#include <linux/input.h>            /* everything */
#include <linux/serio.h>
#include <linux/libps2.h>
#include <linux/printk.h>

MODULE_AUTHOR("Jason Mothafuckin Wilkes");
MODULE_LICENSE("GPL");

/* Scancode to keycode tables */
#define ATKBD_KEYMAP_SIZE	256
static const unsigned short atkbd_my_keycodes[ATKBD_KEYMAP_SIZE] = {
	0,   1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
	64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 99,  0, 86, 87, 88,117,  0,  0, 95,183,184,185,
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	93,  0,  0, 89,  0,  0, 85, 91, 90, 92,  0, 94,  0,124,121,  0,

	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	165,  0,  0,  0,  0,  0,  0,  0,  0,163,  0,  0, 96, 97,  0,  0,
	113,140,164,  0,166,  0,  0,  0,  0,  0,255,  0,  0,  0,114,  0,
	115,  0,172,  0,  0, 98,255, 99,100,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,119,119,102,103,104,  0,105,112,106,118,107,
	108,109,110,111,  0,  0,  0,  0,  0,  0,  0,125,126,127,116,142,
	  0,  0,  0,143,  0,217,156,173,128,159,158,157,155,226,  0,112,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

struct atkbd {
	struct ps2dev ps2dev;
	struct input_dev *dev;	/* include/linux/input.h */
	unsigned char weirdkey;
};

static irqreturn_t atkbd_interrupt(struct serio *serio, unsigned char data, unsigned int flags)
{
	struct atkbd *atkbd = serio_get_drvdata(serio);
	unsigned short code = data & 0b01111111;

	if (data == 0xe0) {
		atkbd->weirdkey = 1;
		goto done;
	}

	input_event(atkbd->dev, EV_KEY, atkbd_my_keycodes[(atkbd->weirdkey) ? (code|0x80) : code], data < 0x80);
	input_sync(atkbd->dev);
	atkbd->weirdkey = 0;
done:	return IRQ_HANDLED;
}

static int atkbd_connect(struct serio *serio, struct serio_driver *drv)
{
	struct atkbd *atkbd;
	int i;

	atkbd = kzalloc(sizeof(struct atkbd), GFP_KERNEL);
	atkbd->dev = input_allocate_device();

	ps2_init(&atkbd->ps2dev, serio);
	serio->dev.driver_data = atkbd; // serio_set_drvdata(serio, atkbd);
	serio_open(serio, drv);

        /* BEGIN SET DEVICE ATTRS */
	atkbd->dev->phys = "isa0060/serio0/input0"; // "isa0060/serio0" == atkbd->ps2dev.serio->phys
	atkbd->dev->name = "The honorable keyboard of Jason Wilkes";
	atkbd->dev->evbit[0]  = 1 << 1; // BIT_MASK(EV_KEY);

	for (i = 0; i < ATKBD_KEYMAP_SIZE; i++)
		set_bit(atkbd_my_keycodes[i], atkbd->dev->keybit);

	input_register_device(atkbd->dev);
	return 0;
}

static void atkbd_disconnect(struct serio *serio)
{
	struct atkbd *atkbd = serio->dev.driver_data; // serio_get_drvdata(serio);
	serio_close(serio);
	input_unregister_device(atkbd->dev);
	kfree(atkbd);
}

static struct serio_device_id atkbd_serio_ids[] = {{.type = SERIO_8042_XL,}, {0}};

static struct serio_driver atkbd_drv = {
	.driver		= {.name = "atkbd"},
	.id_table	= atkbd_serio_ids,
	.interrupt	= atkbd_interrupt,
	.connect	= atkbd_connect,
	.disconnect	= atkbd_disconnect,
};

static int  __init atkbd_init(void) {return serio_register_driver(&atkbd_drv);}
static void __exit atkbd_exit(void) {serio_unregister_driver(&atkbd_drv);}
module_init(atkbd_init);
module_exit(atkbd_exit);
