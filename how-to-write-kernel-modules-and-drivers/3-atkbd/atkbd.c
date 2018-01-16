/* My AT and PS/2 keyboard driver (Was 45K -> now 7K)
 * I've cut this driver down to about 15% of its original size.
 * I probably fucked a few things up, but if so, I can't tell.
 * It still seems to work fine on my laptop. ~ Me
 */

#include <linux/module.h>
#include <linux/slab.h>             /* kzalloc, kfree */
#include <linux/input.h>            /* everything */
#include <linux/serio.h>
#include <linux/libps2.h>
#include <linux/printk.h>

#define DRIVER_DESC	"A stupid keyboard driver"
MODULE_AUTHOR("Jason Mothafuckin Wilkes");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");


/* Scancode to keycode tables */
#define ATKBD_KEYMAP_SIZE	256
static const unsigned short atkbd_keycode[ATKBD_KEYMAP_SIZE] = {
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
#define ATKBD_WEIRD_KEY		0xe0
#define not_yet_initialized(k)	(k->ps2dev.flags & PS2_FLAG_ACK)
#define log()			printk(KERN_DEBUG "[*] %s: entered\n", __func__)

struct atkbd {
	struct ps2dev ps2dev;
	struct input_dev *dev;	/* Defined in include/linux/input.h */
	char phys[32];
	unsigned short keycode[ATKBD_KEYMAP_SIZE];
	unsigned char  weirdkey;
};

/* Process the data received from the keyboard into events.
 * data" is the scancode (e.g., 0x01 for ESC, 0x02 for 1, etc.)
 * When that key is released, it's the same value | 0x80. */
static irqreturn_t atkbd_interrupt(struct serio *serio, unsigned char data, unsigned int flags)
{
	struct atkbd *atkbd = serio_get_drvdata(serio);
	unsigned short code = data & 0b01111111;

	printk(KERN_DEBUG "[*] %s: data == 0x%02x\n", __func__, data);

	if (not_yet_initialized(atkbd)) {
		ps2_handle_ack(&atkbd->ps2dev, data);
		printk(KERN_DEBUG "[*] %s: atkbd not initialized yet. fucking off.\n", __func__);
		return IRQ_HANDLED;
	}

	/* If we've got a weird key (brightness, volume, etc.) then don't
	 * actually do anything yet! Just set atkbd->weirdkey to 1, and do the
 	 * actual processing when the key is *released*! When we look-up
	 * the keycode, check whether atkbd->weirdkey is set. If it is,
 	 * OR the keycode with 0x80 == 128 before we look it up.
	 * Note: My pause/break key can generate 0xe1, but all other
	 * weird keys generate 0xe0. Who needs pause/break anyway? */
	if (data == ATKBD_WEIRD_KEY) {
		atkbd->weirdkey = 1;
		printk(KERN_DEBUG "[*] %s: atkbd->weirdkey set\n", __func__);
		return IRQ_HANDLED;
	}

        /* atkbd_compat_scancode() used to be here. It was a hack almost 
	 * exactly like what Intel did with the Global Descriptor Table */
	input_event(atkbd->dev, EV_KEY, atkbd->keycode[(atkbd->weirdkey) ? (code|0x80) : code], data < 0x80);
	input_sync(atkbd->dev);
	atkbd->weirdkey = 0;
	return IRQ_HANDLED;
}


/* atkbd_connect() is called when the serio module finds an interface
 * that isn't handled yet by an appropriate device driver. We check if
 * there is an AT keyboard out there and if yes, we register ourselves
 * to the input module. */
static int atkbd_connect(struct serio *serio, struct serio_driver *drv)
{
	struct atkbd *atkbd;
	unsigned int scancode;
	int err = -ENOMEM;
	int i;

	log();
	atkbd = kzalloc(sizeof(struct atkbd), GFP_KERNEL);
	atkbd->dev = input_allocate_device();

	ps2_init(&atkbd->ps2dev, serio);

	serio->dev.driver_data = atkbd; // serio_set_drvdata(serio, atkbd);
	serio_open(serio, drv);

        /* BEGIN ATKBD PROBE */
	printk(KERN_DEBUG "[*] %s: this will cause a keyboard interrupt\n", __func__);
	if (ps2_command(&atkbd->ps2dev, NULL, ATKBD_CMD_ENABLE)) {
		printk(KERN_INFO "[*] %s: ps2_command returned nonzero. Bailing out!\n", __func__);
		err = -ENODEV;
                goto fail;
	}
	printk(KERN_DEBUG "[*] %s: returning from keyboard interrupt\n", __func__);

        /* BEGIN SET KEYCODE TABLE */
	printk(KERN_DEBUG "[*] %s: in former atkbd_set_keycode_table\n", __func__);
	for (i = 0; i < 128; i++) {
		scancode 		= atkbd_unxlate_table[i];
		atkbd->keycode[i] 	= atkbd_keycode[scancode];
		atkbd->keycode[i|0x80]  = atkbd_keycode[scancode|0x80];
	}

        /* BEGIN SET DEVICE ATTRS */
	printk(KERN_DEBUG "[*] %s: in former atkbd_set_device_attrs\n", __func__);
	snprintf(atkbd->phys, sizeof(atkbd->phys), "%s/input0", atkbd->ps2dev.serio->phys);
	atkbd->dev->phys = atkbd->phys;
	atkbd->dev->name = "The honorable keyboard of Jason Wilkes";
	atkbd->dev->dev.driver_data = atkbd; // same as input_set_drvdata(atkbd->dev, atkbd);
	atkbd->dev->evbit[0]  = BIT_MASK(EV_KEY);

        /* Print some of the info we just set */
        printk(KERN_DEBUG "[*] %s: atkbd->dev->name == %s\n", __func__, atkbd->dev->name);
        printk(KERN_DEBUG "[*] %s: atkbd->dev->phys == %s\n", __func__, atkbd->dev->phys);

	for (i = 0; i < ATKBD_KEYMAP_SIZE; i++)
		__set_bit(atkbd->keycode[i], atkbd->dev->keybit);

	input_register_device(atkbd->dev);
	return 0;

fail:
	serio_close(serio);
	serio->dev.driver_data = NULL;
	input_free_device(atkbd->dev);
	kfree(atkbd);
	return err;
}

static void atkbd_disconnect(struct serio *serio)
{
	struct atkbd *atkbd = serio_get_drvdata(serio);
	log();
	serio_close(serio);
	serio->dev.driver_data = NULL;
	input_unregister_device(atkbd->dev);
	kfree(atkbd);
}

static struct serio_device_id atkbd_serio_ids[] = {
	{.type	= SERIO_8042_XL,},
	{0}
};

static struct serio_driver atkbd_drv = {
	.driver		= {.name = "atkbd"},
	.id_table	= atkbd_serio_ids,
	.interrupt	= atkbd_interrupt,
	.connect	= atkbd_connect,
	.disconnect	= atkbd_disconnect,
};

static int  __init atkbd_init(void) {log(); return serio_register_driver(&atkbd_drv);}
static void __exit atkbd_exit(void) {log(); serio_unregister_driver(&atkbd_drv);}
module_init(atkbd_init);
module_exit(atkbd_exit);

