#include <linux/module.h>	/* struct module, container_of */
#include <linux/list.h>		/* list_for_each_entry */


extern void modules_lsmod(void)
{
	struct module *mod = THIS_MODULE; // mod = find_module("waffles") also works
	/* Set the list head to that weirdo non-module.
	 * Then we don't need to treat it as a special case */
	struct list_head *modules = mod->list.prev;

	list_for_each_entry(mod, modules, list) {
		printk(KERN_DEBUG "%s: %-24s at %p\n", __func__, mod->name, mod);
	}
}

/* Nothing below this is currently used. Just keeping it around for reference. */

static void modules_lsmod_v1(void)
{
	struct module *mod = THIS_MODULE;	// mod = find_module("waffles") also works
	struct list_head *modules = &mod->list; // &THIS_MODULE->list also works

	list_for_each_entry(mod, modules, list) {
		printk(KERN_DEBUG "%s: module %-32s at %p\n", __func__, mod->name, mod);
	}
}


static void modules_lsmod_v2(void)
{
	struct module *mod = THIS_MODULE;
	struct list_head *modules;

	mod = container_of(mod->list.next, struct module, list);
	for (modules = &mod->list; mod != THIS_MODULE; mod = container_of(mod->list.next, struct module, list)) {
		printk(KERN_DEBUG "%s: module %-32s at %p\n", __func__, mod->name, mod);
	}
}


static void modules_lsmod_v3(void)
{
	struct module *mod = THIS_MODULE;
	do {
		printk(KERN_DEBUG "%s: module %-32s at %p\n", 
			__func__, mod->name, mod);
		mod = container_of(mod->list.next, struct module, list);
	} while (mod != THIS_MODULE);
}


static void modules_lsmod_v4(void)
{
	struct module *mod = THIS_MODULE;	// mod = find_module("waffles") also works
	struct list_head *modules = &(mod->list); // &THIS_MODULE->list also works

	list_for_each_entry(mod, modules, list) {
		 if (mod->name[0] == 1)
			continue;
		printk(KERN_DEBUG "%s: %-24s at %p\n", __func__, mod->name, mod);
		/* also useful: mod->init and mod->exit */
	}
}


/* Put this in a loop over all modules to examine a module struct's memory */
/*
int i;
if (mod->name[0] == 1) {
	for (i = 0; i < sizeof(struct module); i++) {
		printk("%02x ", *((unsigned char *)mod + i));
	}
	printk("\n\n");
	printk("init: %p\n", mod->init);
	printk("exit: %p\n", mod->exit);
}
*/
