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
#include <linux/ioport.h>
#include <asm/uaccess.h>    /* copy_from/to_user */
#include <asm/io.h>         /* inb, outb */

#define BUFSIZE 1
//#define IOPORT  0xf040
//#define IOPORT  0xe000
//#define IOPORT  0x03f8
#define IOPORT  0x0378
MODULE_LICENSE("GPL");

/* Function declaration of parlelport.c */ 
int parlelport_open(struct inode *inode, struct file *filp);
int parlelport_release(struct inode *inode, struct file *filp);
ssize_t parlelport_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t parlelport_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
void parlelport_exit(void);
int parlelport_init(void);

/* Structure that declares the common file access functions */
struct file_operations parlelport_fops = { 
    read:    parlelport_read,
    write:   parlelport_write,
    open:    parlelport_open,
    release: parlelport_release
};

/* Driver global variables */
int parlelport_major = 61;  /* Major number */
int port;                   /* Control variable for memory reservation of the parallel port*/
long loc = 0;               /* Location within the (trivial) kernel space buffer */
unsigned char parlelport_buffer;     /* Buffer to read the device */

module_init(parlelport_init);
module_exit(parlelport_exit);


/* Mine */
void parlelport_cleanup(void)
{
    memset(&parlelport_buffer, 0, BUFSIZE);
    loc = 0;
}


int parlelport_init(void) { 
    int result;

    /* Registering device */
    result = register_chrdev(parlelport_major, "parlelport", &parlelport_fops);
    if (result < 0) { 
        printk("<1> parlelport: cannot obtain major number %d\n", parlelport_major); 
        return result;
    } 

    /* Registering port */
    port = check_region(IOPORT, 1);
    if (port) { 
        printk("<1> parlelport: cannot reserve IOPORT\n");
        result = port;
        goto fail;
    } 
    request_region(IOPORT, 1, "parlelport");

    printk("<1> Inserting parlelport module\n"); 
    return 0;

    fail: 
        parlelport_exit(); 
        return result;
}


void parlelport_exit(void) {

    /* Make major number free! */
    unregister_chrdev(parlelport_major, "parlelport");

    /* Make port free! */ 
    if (!port) { 
        release_region(IOPORT, 1);
    }

    printk("<1> Removing parlelport module\n");
}


int parlelport_open(struct inode *inode, struct file *filp) {
    return 0;       /* Success */
}


int parlelport_release(struct inode *inode, struct file *filp) {
    return 0;       /* Success */
}


ssize_t parlelport_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {

    /* Reading port */
    parlelport_buffer = inb(IOPORT);
    printk("In read, parlelport_buffer = %c\n", parlelport_buffer);

    /* We transfer data to user space */
    copy_to_user(buf, &parlelport_buffer, BUFSIZE);
    return 0;
    if (*f_pos == 0) {
        *f_pos += BUFSIZE;
        return BUFSIZE;
    } else {
        parlelport_cleanup();
        return 0;
    }
}


ssize_t parlelport_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {

    /* Read data from user space */
    copy_from_user(&parlelport_buffer + loc, buf, 1);

    loc++;  /* Increment our location within the kernel-space buffer by one */
    buf++;  /* Increment the location of the user-space buffer by one */

    /* Writing to the port! */
    outb((unsigned char)parlelport_buffer, (unsigned short int)IOPORT);
    printk("In write, parlelport_buffer = %c\n", parlelport_buffer);

    /* Here's where the mystery begins... */
    printk("After writing, *(0x%x) = %02x\n", IOPORT, (unsigned char)inl(IOPORT));
    return 1;

}
