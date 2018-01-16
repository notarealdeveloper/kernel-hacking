#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include "kapi.h"

/* For more fun things to do, cat System.map
 * and grep for interesting words, (e.g., "current") */

MODULE_LICENSE("GPL");

static void kmod_exit(void);
static void kmod_exit(void);
static void kmod_get_retarded(void);

static int kmod_init(void) 
{
	msg("Entering module");
	kobj_init();
	modules_lsmod();
	context_print();
	kmod_get_retarded();
	procs_print_task(NULL);
	procs_psaux();
	return 0;
}

static void kmod_get_retarded(void)
{
	void kmod_exit_hook(void)
	{
		msg("Haha gotcha bitch!");
		kmod_exit();
	}

	/* install our exit hook */
	/* note: these functions end up in the text segment,
	 * not on the stack, so this should be perfectly safe :D */
	THIS_MODULE->exit = kmod_exit_hook;
}

static void kmod_exit(void)
{
	kobj_kill();
	procs_print_task(NULL);
	msg("Leaving module");
}

module_init(kmod_init);
module_exit(kmod_exit);
