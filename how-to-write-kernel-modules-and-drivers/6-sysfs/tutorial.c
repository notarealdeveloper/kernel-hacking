#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

#define msg(fmt, args...) \
	printk(KERN_INFO "%s: " fmt "\n", __func__, ## args)

/* Rules of sysfs: (1) One value per file. (2) That value is in ascii. */

static int __init tutorial_init(void)
{
	msg("Entering module");
	return 0;
}

static void __exit tutorial_exit(void)
{
	msg("Leaving module");
}

module_init(tutorial_init);
module_exit(tutorial_exit);

MODULE_AUTHOR("Jason Wilkes");
MODULE_DESCRIPTION("Various waffles");
MODULE_LICENSE("GPL");
