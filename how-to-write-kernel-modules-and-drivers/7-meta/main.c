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

static void kmod_exit(void);
static void kmod_exit(void);
static void kmod_get_retarded(void);

static int kmod_init(void) 
{
	msg("[*] Initializing module");
	kobj_init();
	modules_lsmod();
	context_print();
	kmod_get_retarded();
	procs_print_task(NULL);
	procs_psaux();
	setup_regs_for_test();
	state_dump();
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
	kobj_kill();
	toplel_remove();
	msg("[*] Done removing module");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jason Wilkes");

module_init(kmod_init);
module_exit(kmod_exit);
