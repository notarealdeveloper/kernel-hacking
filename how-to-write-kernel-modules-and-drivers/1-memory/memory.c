/* 
 * MY FIRST DEVICE DRIVER 
 * ======================
 * Original code taken from a tutorial,
 * then heavily modified to support new features.
 * ~Jason 
 */

/* Necessary includes for device drivers */
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

#define DEBUG       0
#define BUFSIZE     1024
#define fmt(x)      \
        (void *)(((long)(x) % 0x0000000000010000) % 0xffffffffffff0000) /* For formatted debugging output */

MODULE_LICENSE("GPL");

/* Declaration of memory.c functions */
int     memory_open     (struct inode *inode, struct file *filp);
int     memory_release  (struct inode *inode, struct file *filp);
ssize_t memory_read     (struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t memory_write    (struct file *filp, const char *buf, size_t count, loff_t *f_pos);
void    memory_exit     (void);
int     memory_init     (void);


/* Structure that declares the usual file */
/* access functions */
struct file_operations memory_fops = {
    read: memory_read,
    write: memory_write,
    open: memory_open,
    release: memory_release
};


/* Declaration of the init and exit functions */
module_init(memory_init);
module_exit(memory_exit);


/* Global variables of the driver */
int memory_major = 60;      /* Major number */
char *memory_buffer;        /* Buffer to store data */
long loc = 0;               /* Location within kernel-space buffer */



/*********************************************/
/*********** BEGIN MY FUNCTIONS **************/
/*********************************************/

/* 
 * Clear the device after we finish reading from it.
 * This makes the buffer work like an iterator in Python.
 * I wrote this :) ~Jason
 */
void memory_cleanup(void)
{
    memset(memory_buffer, 0, BUFSIZE);
    loc = 0;    /* Reset our position within the kernel-space buffer to zero */
}


/* Print information to the dmesg buffer that might be helpful during debugging */
void print_debug_info(const char *context, const char *buf)
{
    /* Note: We print the current character from buf, but we always print 
     * the entire contents of memory_buffer as a string. The latter is safe. */
    printk("<1> %s: buf = %4p, *(buf) = %c\t", context, fmt(buf), *buf);
    printk("memory_buffer = %4p, *(memory_buffer) = %s\n", fmt(memory_buffer), memory_buffer);
}

/*********************************************/
/************ END MY FUNCTIONS ***************/
/*********************************************/


/* Memory init */
int memory_init(void) {
    int result;

    /* Registering device */
    result = register_chrdev(memory_major, "memory", &memory_fops);
    if (result < 0) {
        printk("<1> memory: cannot obtain major number %d\n", memory_major);
    return result;
    }

    /* Allocating memory for the buffer */
    memory_buffer = kmalloc(BUFSIZE, GFP_KERNEL);
    if (!memory_buffer) {
        result = -ENOMEM;
        goto fail;
    }
    memset(memory_buffer, 0, BUFSIZE);

    printk("<1> Inserting memory module\n"); 
    return 0;

    fail: 
        memory_exit(); 
        return result;
}



/* Memory exit */
void memory_exit(void) {
    /* Freeing the major number */
    unregister_chrdev(memory_major, "memory");

    /* Freeing buffer memory */
    if (memory_buffer) {
        kfree(memory_buffer);
    }
    printk("<1> Removing memory module\n");
}



/* Memory open */
int memory_open(struct inode *inode, struct file *filp) {
    return 0;
}



/* Memory release */
int memory_release(struct inode *inode, struct file *filp) {
    return 0;
}


/* Memory read */
ssize_t memory_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) { 

    /* Transfering data to user space */
    copy_to_user(buf, memory_buffer, BUFSIZE);

    if (DEBUG) { print_debug_info("read aft", (const char *)buf); }

    /* Changing reading position as best suits */ 
    if (*f_pos == 0) { 
        *f_pos += BUFSIZE;
        return BUFSIZE; /* I think read functions must (or usually) return the number of bytes read */
    } 
    else {
        memory_cleanup();
        return 0;
    }
}


/* Memory write */
ssize_t memory_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {

    if (DEBUG) { print_debug_info("wri bef", buf); }

    copy_from_user(memory_buffer + loc, buf, 1);

    if (DEBUG) { print_debug_info("wri aft", buf); }

    loc++;  /* Increment our location within the kernel-space buffer by one */
    buf++;  /* Increment the location of the user-space buffer by one */
    return 1;
}
