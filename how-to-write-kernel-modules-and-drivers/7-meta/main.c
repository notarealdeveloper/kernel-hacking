#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include "kapi.h"
#include "state.h"
#include "toplel.h"

/* More fun things to do:
 * (1) cat System.map, and grep for interesting terms
 * (2) visit http://en.wikipedia.org/wiki/Magic_SysRq_key
 */

/* fs/proc/cmdline.c is a *great* minimal example of a /proc file */

static int  kmod_init(void);
static void kmod_exit(void);
static void kmod_get_retarded(void);

static int __init kmod_init(void) 
{
	msg("[*] Initializing module");
	sysfs_enter();
	procfs_init();
	// modules_lsmod();
	context_print();
	kmod_get_retarded();
	procs_print_task(NULL);
	// procs_psaux();
	setup_regs_for_test();
	state_dump();
	uaccess_show_access_info();
	uaccess_show_thread_info();
	lib_glob_usage();
	lib_syscall_info();
	msg("[*] Done initializing module");
	return 0;
}

static void kmod_get_retarded(void)
{
	void kmod_exit_hook(void)
	{
		msg("Haha gotcha bitch!");
		kmod_exit();
	}
	/* install our module exit hook */
	THIS_MODULE->exit = kmod_exit_hook;
}

static void kmod_exit(void)
{
	msg("[*] Removing module");
	procs_print_task(NULL);
	sysfs_leave();
	procfs_exit();
	toplel_remove();
	msg("[*] Done removing module");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jason Wilkes");

module_init(kmod_init);
module_exit(kmod_exit);
