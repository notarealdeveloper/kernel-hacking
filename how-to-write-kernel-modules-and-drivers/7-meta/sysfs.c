/* 
 * How to use sysfs and kobjects.
 * Adapted from the samples/ directory of the kernel tree
 *
 * This makes a subdirectory in sysfs called /sys/kernel/kobject-example.
 * In that directory, 3 files are created: "foo", "baz", and "bar".
 * If an integer is written to these files, it can be read out of it later.
 * However, each file changes its integer in its own way whenever we read it.
 */

#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>

#include <linux/reboot.h>	/* for doing retarded shit  :) 	*/
#include "kapi.h"		/* for get_process_info() 	*/


static int foo;

static ssize_t foo_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	procs_print_task(NULL);
	return sprintf(buf, "%u\n", foo++);
}

static ssize_t foo_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	procs_print_task(NULL);
	sscanf(buf, "%u", &foo);

	/* If we echo 666 into our foo "file", the machine reboots ;) */
	if (foo == 666)
		kernel_restart(NULL);

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
 * kobject-example/mydir, rather kobject-example */
static struct attribute_group attr_group = {
	.attrs = attrs,
};

static struct kobject *my_kobj;


/* Exported functions begin here */
extern int kobj_init(void)
{
	int ret;

	/* Create /sys/kernel/kobject-example */
	my_kobj = kobject_create_and_add("kobject-example", kernel_kobj);
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
