/* Based on info from http://bit.ly/1GNbY5n
 * Other sources with similar information:
 * http://www.gadgetweb.de/linux/40-how-to-hijacking-the-syscall-table-on-latest-26x-kernel-systems.html
 * http://kerneltrap.org/mailarchive/linux-kernel/2008/1/25/612014
 * http://badishi.com/kernel-writing-to-read-only-memory/
 * 
 * Another great source: https://github.com/jvns/kernel-module-fun
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>
#include <linux/syscalls.h>
#include <linux/delay.h>    /* loops_per_jiffy */


/* This is the famous write protect bit (CR0:16) in control register 0 */
#define CR0_WP 0x00010000
#define DISABLE_WRITE_PROTECTION()  (write_cr0(read_cr0() & (~ CR0_WP)))
#define ENABLE_WRITE_PROTECTION()   (write_cr0(read_cr0() | CR0_WP))

MODULE_LICENSE("GPL");


void **syscall_table;
unsigned long **find_sys_call_table(void);
long (*orig_sys_open)(const char __user *filename, int flags, int mode);


/* look for sys_call_table */
unsigned long **find_sys_call_table() {
    
    unsigned long ptr;
    unsigned long *p;

    printk(KERN_DEBUG "[*] In toplel : %s\n", __func__);

    for (ptr = (unsigned long)sys_close;
         ptr < (unsigned long)&loops_per_jiffy;
         ptr += sizeof(void *)) {

        p = (unsigned long *)ptr;

        if (p[__NR_close] == (unsigned long)sys_close) {
            printk(KERN_DEBUG "[+] Found the sys_call_table :D\n");
            return (unsigned long **)p;
        }
    }

    printk(KERN_DEBUG "[-] Couldn't find the sys_call_table :(\n");
    return NULL;
}

/* our hook for sys_open */
long my_sys_open(const char __user *filename, int flags, int mode) {

    long ret;
    ret = orig_sys_open(filename, flags, mode);

    printk(KERN_DEBUG "[+] File %s opened with mode %d\n", filename, mode);
    return ret;
}


/* module init */
static int __init toplel_init(void)
{
    int ret;
    unsigned long addr;

    printk(KERN_DEBUG "[*] In toplel : %s\n", __func__);

    syscall_table = (void **)find_sys_call_table();

    /* If we failed to find the syscall table, bail out. */
    if (!syscall_table)
        return -1;

   /* Alright! If we're here, then everything's coming up Milhouse.
    * Now, disable write-protection by flipping the WP bit in cr0 */
    DISABLE_WRITE_PROTECTION();

    /* Set the syscall_table to be rw. 3 pages should be enough. */
    addr = (unsigned long)syscall_table;

    // Signature is int set_memory_rw(unsigned long addr, int numpages)
    ret = set_memory_rw(PAGE_ALIGN(addr) - PAGE_SIZE, 3);
    if (ret)
        printk(KERN_DEBUG "[-] Failed to set mem rw (%d) at addr %16lx\n", ret, PAGE_ALIGN(addr) - PAGE_SIZE);
    else
        printk(KERN_DEBUG "[+] Win! 3 pages set to rw\n");

    /* Install our hook! :D */
    orig_sys_open = syscall_table[__NR_open];
    syscall_table[__NR_open] = my_sys_open;
    printk(KERN_DEBUG "[*] Finished installing our hook!\n");


    /* Turn write-protection back on, to prevent general anarchy. */
    ENABLE_WRITE_PROTECTION();
  
    return 0;
}


/* module exit */
static void __exit toplel_exit(void)
{
    printk(KERN_DEBUG "[*] In toplel : %s\n", __func__);

    /* Uninstall our hook. Have to turn-off write-protection first. */
    DISABLE_WRITE_PROTECTION();
    syscall_table[__NR_open] = orig_sys_open;  
    ENABLE_WRITE_PROTECTION();
}

module_init(toplel_init);
module_exit(toplel_exit);
