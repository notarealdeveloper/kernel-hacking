/* Based on info from http://bit.ly/1GNbY5n
 * Another great source: https://github.com/jvns/kernel-module-fun
 * Other sources with similar information, which I haven't checked-out yet:
 * http://www.gadgetweb.de/linux/40-how-to-hijacking-the-syscall-table-on-latest-26x-kernel-systems.html
 * http://kerneltrap.org/mailarchive/linux-kernel/2008/1/25/612014
 * http://badishi.com/kernel-writing-to-read-only-memory/
 */


#include <linux/types.h>
#include <linux/syscalls.h>
#include <linux/delay.h>    /* loops_per_jiffy */
#include <linux/string.h>   /* strlen, strcmp */

#include "toplel.h"
#include "util.h"

/* This is the famous write protect bit (CR0:16) in control register 0 */
#define CR0_WP                      0x00010000
#define DISABLE_WRITE_PROTECTION()  (write_cr0(read_cr0() & ~CR0_WP))
#define ENABLE_WRITE_PROTECTION()   (write_cr0(read_cr0() |  CR0_WP))


static char *types[] = {"mp3", "jpg", "gif", "pdf", NULL};
static void **syscall_table;
static long (*orig_sys_open)(const char __user *filename, int flags, int mode);
static bool toplel_installed = false;


/* look for the sys_call_table */
static unsigned long **find_sys_call_table(void)
{   
	unsigned long i, *p;

	msg("[*] Looking for the syscall table");
	for (i = (unsigned long)sys_close; i < (unsigned long)&loops_per_jiffy; i += sizeof(void *)) {
		p = (unsigned long *)i;
		if (p[__NR_close] == (unsigned long)sys_close) {
			msg("[+] Found the sys_call_table :D");
			return (unsigned long **)p;
		}
	}
	msg("[-] Couldn't find the sys_call_table :(");
	return NULL;
}

static char *fun_filetype(const char __user *filename)
{
	int i;
	for (i = 0; types[i] != NULL; i++) {
		if (!strcmp(filename + strlen(filename) - 3, types[i]))
			return types[i];
	}
	return (char *)NULL;
}

asmlinkage long my_sys_open(const char __user *filename, int flags, umode_t mode)
{
	mm_segment_t	old_fs;
	long		fd;
	char		*filetype;
	char		hookfile[256]; // strlen(getcwd)

	filetype = fun_filetype(filename);

	/* If it's not one of our "fun" files, use the default sys_open */
	if (!filetype)
		return orig_sys_open(filename, flags, mode);

	/* Hey! it's one of the good filetypes. Let's have some fun :D
	* ========================================= */

	/* use my getcwd() macro to make a string containing the hookfile data dir  */
	snprintf(hookfile, sizeof(hookfile), "%s/toplel/toplel.%s", getcwd(), filetype);

	msg("[*] Opening hook file %s", hookfile);

	/* sys_open checks to see if the filename is a pointer to user space
	 * memory. When we're hijacking, the filename we pass will be in kernel
	 * memory. To get around this, we juggle some segment registers. I
	 * believe fs is the segment used for user space, and we're temporarily
	 * changing it to be the segment the kernel uses.
	 *
	 * An alternative would be to use read_from_user() and copy_to_user()
	 * and place the hooked filename at the location the user code passed
	 * in, saving and restoring the memory we overwrite. */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fd = orig_sys_open(hookfile, flags, mode);

	/* Restore fs to its original value */
	set_fs(old_fs);

	return fd;
}

/* install the hook */
int toplel_insert(void)
{
	unsigned long addr;
	if (toplel_installed)
		return -1;

	msg("[*] Installing hook");
	syscall_table = (void **)find_sys_call_table();

	/* If we failed to find the syscall table, bail out. */
	if (!syscall_table)
		return -1;

	/* Alright! If we're here, then everything's coming up Milhouse.
	* Now, disable write-protection by flipping the WP bit in cr0 */
	DISABLE_WRITE_PROTECTION();

	/* Set the syscall_table to be rw. 3 pages should be enough. */
	addr = (unsigned long)syscall_table;

	/* Install our hook! :D */
	orig_sys_open = syscall_table[__NR_open];
	syscall_table[__NR_open] = my_sys_open;
	msg("[*] Hook installed!");

	/* Turn write-protection back on, to prevent general anarchy. */
	ENABLE_WRITE_PROTECTION();

	toplel_installed = true;

	return 0;
}


/* restore the syscall table to its original state */
void toplel_remove(void)
{
	if (!toplel_installed)
		return;
	msg("[*] Removing hook");
	/* Uninstall our hook. Have to turn-off write-protection first. */
	DISABLE_WRITE_PROTECTION();
	syscall_table[__NR_open] = orig_sys_open;  
	ENABLE_WRITE_PROTECTION();
	toplel_installed = false;
	return;
}

