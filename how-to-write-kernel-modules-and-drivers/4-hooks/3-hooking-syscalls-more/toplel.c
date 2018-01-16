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
#include <linux/syscalls.h>
#include <linux/delay.h>    /* loops_per_jiffy */
#include <linux/string.h>   /* strlen, strcmp */

#include <linux/sched.h>    /* for task_struct (current) */

/* This is the famous write protect bit (CR0:16) in control register 0 */
#define CR0_WP                      0x00010000
#define DISABLE_WRITE_PROTECTION()  (write_cr0(read_cr0() & ~CR0_WP))
#define ENABLE_WRITE_PROTECTION()   (write_cr0(read_cr0() |  CR0_WP))

MODULE_LICENSE("GPL");


void **syscall_table;
unsigned long **find_sys_call_table(void);
long (*orig_sys_open) (const char __user *filename, int flags, int mode);
long (*orig_sys_mkdir)(const char __user *pathname, umode_t mode);
long (*orig_sys_rmdir)(const char __user *pathname);


/* The Makefile generates rickroll.h, which declares a static char *
 * named rickroll_filepath, and declares it to be the absolute path
 * of ./never-gonna-give-you-up.mp3 */
#include "rickroll.h"


/* look for the sys_call_table */
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


/* our replacement for sys_mkdir */
asmlinkage long my_sys_mkdir(const char __user *pathname, umode_t mode)
{
    printk("[+] %s : directory %s created (mode %d)\n", __func__, pathname, mode);
    return orig_sys_mkdir(pathname, mode);
}


/* our replacement for sys_rmdir */
asmlinkage long my_sys_rmdir(const char __user *pathname)
{
    printk("[+] %s : directory %s removed\n", __func__, pathname);
    return orig_sys_rmdir(pathname);
}


/* our replacement for sys_open */
asmlinkage long my_sys_open(const char __user *filename, int flags, umode_t mode)
{
    mm_segment_t    old_fs;
    long            fd;

    struct task_struct *cur = current;

    /* Note: /proc is examined so often that it blows-up the
     * system if we try to printk every file access. */

    //if (strncmp(filename, "/proc", 5) && strncmp(filename, "/sys", 4))
    if (!strncmp(filename, "/home", 5) || !strncmp(filename, "/usr/bin", 8)) {
        printk("[+] %s : file %s opened by pid %d\n", __func__, filename, cur->pid);
    }

    /* If it's not an mp3, use the default sys_open */
    if (strcmp(filename + strlen(filename) - 4, ".mp3"))
        return orig_sys_open(filename, flags, mode);


   /* Hey! it's an mp3. Let's have some fun :D
    * ========================================= */

   /* sys_open checks to see if the filename is a pointer to user space
    * memory. When we're hijacking, the filename we pass will be in kernel
    * memory. To get around this, we juggle some segment registers. I
    * believe fs is the segment used for user space, and we're temporarily
    * changing it to be the segment the kernel uses. */
    old_fs = get_fs();
    set_fs(KERNEL_DS);

    /* Open the rickroll file instead */
    fd = orig_sys_open(rickroll_filepath, flags, mode);

    /* Restore fs to its original value */
    set_fs(old_fs);

    return fd;
}


/* module init */
static int __init toplel_init(void)
{
    printk(KERN_DEBUG "[*] In toplel : %s\n", __func__);

    syscall_table = (void **)find_sys_call_table();

    /* If we failed to find the syscall table, bail out. */
    if (!syscall_table)
        return -1;

   /* Alright! If we're here, then everything's coming up Milhouse.
    * Now, disable write-protection by flipping the WP bit in cr0 */
    DISABLE_WRITE_PROTECTION();

    /* Save the old syscalls for posterity */
    orig_sys_open  = syscall_table[__NR_open];
    orig_sys_mkdir = syscall_table[__NR_mkdir];
    orig_sys_rmdir = syscall_table[__NR_rmdir];

    /* Install our hook! :D */
    syscall_table[__NR_open]  = my_sys_open;
    syscall_table[__NR_mkdir] = my_sys_mkdir;
    syscall_table[__NR_rmdir] = my_sys_rmdir;

    /* Turn write-protection back on, to prevent general anarchy. */
    ENABLE_WRITE_PROTECTION();
  
    printk(KERN_DEBUG "[*] Finished installing our hooks!\n");
    return 0;
}


/* module exit */
static void __exit toplel_exit(void)
{
    printk(KERN_DEBUG "[*] In toplel : %s\n", __func__);

    /* Uninstall our hooks */
    DISABLE_WRITE_PROTECTION();
    syscall_table[__NR_open]  = orig_sys_open;
    syscall_table[__NR_mkdir] = orig_sys_mkdir;
    syscall_table[__NR_rmdir] = orig_sys_rmdir;
    ENABLE_WRITE_PROTECTION();
}

module_init(toplel_init);
module_exit(toplel_exit);
