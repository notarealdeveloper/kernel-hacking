#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>   /* printk() */
#include <linux/slab.h>     /* kmalloc() */
#include <linux/fs.h>       /* everything... */
#include <linux/errno.h>    /* error codes */
#include <linux/types.h>    /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>    /* O_ACCMODE */
#include <asm/uaccess.h>    /* copy_from/to_user */

#define BUFSIZE     1024*50
#define fmt(x)      \
        (void *)(((long)(x) % 0x0000000000010000) % 0xffffffffffff0000) /* For formatted debugging output */

MODULE_LICENSE("GPL");

int     memory_open     (struct inode *inode, struct file *filp);
int     memory_release  (struct inode *inode, struct file *filp);
ssize_t memory_read     (struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t memory_write    (struct file *filp, const char *buf, size_t count, loff_t *f_pos);
void    memory_exit     (void);
int     memory_init     (void);

struct file_operations memory_fops = {
    read:    memory_read,
    write:   memory_write,
    open:    memory_open,
    release: memory_release
};

module_init(memory_init);
module_exit(memory_exit);


/* Global variables of the driver */
int memory_major = 60;      /* Major number */
char *memory_buffer;        /* Buffer to store data */
long loc = 0;               /* Location within kernel-space buffer */



/*********** BEGIN MY FUNCTIONS **************/
void memory_cleanup(void)
{
    memset(memory_buffer, 0, BUFSIZE);
    loc = 0;
}

void print_debug_info(const char *context, const char *buf)
{
    printk("<1> %s: buf = %4p, *(buf) = %c\t", context, fmt(buf), *buf);
    printk("memory_buffer = %4p, *(memory_buffer) = %s\n", fmt(memory_buffer), memory_buffer);
}
/************ END MY FUNCTIONS ***************/


int memory_init(void) {
    int result;
    result = register_chrdev(memory_major, "memory", &memory_fops);
    /* Allocating memory for the buffer */
    memory_buffer = kmalloc(BUFSIZE, GFP_KERNEL);
    memset(memory_buffer, 0, BUFSIZE);
    printk("<1> Inserting memory module\n"); 
    return 0;
}

void memory_exit(void) {
    /* Freeing the major number and buffer memory */
    unregister_chrdev(memory_major, "memory");
    if (memory_buffer) kfree(memory_buffer);
    printk("<1> Removing memory module\n");
}

int memory_open(struct inode *inode, struct file *filp) {return 0;}
int memory_release(struct inode *inode, struct file *filp) {return 0;}

ssize_t memory_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) { 
    copy_to_user(buf, memory_buffer, BUFSIZE);     /* Transfering data to user space */
    if (*f_pos == 0) {*f_pos += BUFSIZE; return BUFSIZE;}
    else {memory_cleanup(); return 0;}
}


/* Memory write */
ssize_t memory_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
    copy_from_user(memory_buffer + loc, buf, 1);
    loc++;  /* Increment our location within the kernel-space buffer by one */
    buf++;  /* Increment the location of the user-space buffer by one */
    return 1;
}
