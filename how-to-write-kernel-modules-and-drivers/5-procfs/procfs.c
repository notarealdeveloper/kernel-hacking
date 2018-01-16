#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>

/* for proc_fs stuff */
#include <linux/proc_fs.h>	/* struct proc_dir_entry, and everything else */
#include <linux/fs.h>		/* struct file_operations */
#include <asm/uaccess.h>	/* copy_{to,from}_user */

#define PROCFILE	"my-proc-file"
#define BUFSIZE		1024


static struct proc_dir_entry *proc_file;
static struct file_operations proc_fops;
static char *kbuf;

static ssize_t procfs_reader(struct file *filp, char __user *buf, size_t count, loff_t *pos)
{
	/* note: we always have *pos == strlen(buf) */
	if (*pos)
		return 0;
	*pos = sprintf(buf, "%s", kbuf);
	return *pos;
}

static ssize_t procfs_writer(struct file *filp, const char __user *buf, size_t count, loff_t *pos)
{
	size_t nbytes = (count < BUFSIZE) ? (count + 1) : BUFSIZE;
	*pos = 0; /* truncate the "file" */
	snprintf(kbuf, nbytes, "%s", buf);
	return count;
}

static int __init procfs_init(void)
{
	printk(KERN_DEBUG "Inserting module\n");
	kbuf = kmalloc(BUFSIZE, GFP_KERNEL);
	proc_file = proc_create(PROCFILE, 0664, NULL /* parent */, &proc_fops);
	return 0;
}

static void __exit procfs_exit(void)
{
	printk(KERN_DEBUG "Removing module\n");
	proc_remove(proc_file);
	kfree(kbuf);
}

static struct file_operations proc_fops = {
	.owner 	= THIS_MODULE,
	.read 	= procfs_reader,
	.write	= procfs_writer,
};

module_init(procfs_init);
module_exit(procfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jason Wilkes <letshaveanadventure@gmail.com>");
MODULE_DESCRIPTION("A minimal kernel module showing how to make a file in /proc");

