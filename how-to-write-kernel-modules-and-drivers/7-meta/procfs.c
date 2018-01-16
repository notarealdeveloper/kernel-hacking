#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>

/* for proc_fs stuff */
#include <linux/proc_fs.h>	/* struct proc_dir_entry, and everything else */
#include <linux/fs.h>		/* struct file_operations */
#include <asm/uaccess.h>	/* copy_{to,from}_user */

#include "util.h"

#define PROCFILE	"my-proc-file"
#define BUFSIZE		1024

/* TODO: Add procdir = proc_mkdir("myprocdir", NULL); */

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

int procfs_init(void)
{
	msg("Creating /proc file");
	kbuf = kmalloc(BUFSIZE, GFP_KERNEL);
	proc_file = proc_create(PROCFILE, 0664, NULL /* parent */, &proc_fops);
	return 0;
}

void procfs_exit(void)
{
	msg("Removing /proc file");
	proc_remove(proc_file);
	kfree(kbuf);
}

static struct file_operations proc_fops = {
	.owner 	= THIS_MODULE,
	.read 	= procfs_reader,
	.write	= procfs_writer,
};

