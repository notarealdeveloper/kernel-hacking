/* defines that should be generally helpful */
#define msg(s) 	printk(KERN_DEBUG "%s: " s "\n", __func__)

/* includes for process related stuff */
#include <linux/sched.h>	/* for current */
void procs_print_task(struct task_struct *task);
void procs_print_children(struct task_struct *task);
void procs_psaux(void);

/* includes for in_interrupt()
 * [also pulled-in by something else] */
#include <linux/preempt_mask.h>
#define context_print() ({				\
	if (in_interrupt())				\
		msg("Running in interrupt context");	\
	else						\
		msg("Running in process context");	\
})


/* includes for sysfs.c */
extern int  kobj_init(void);
extern void kobj_kill(void);


/* includes for modules.c */
extern void modules_lsmod(void);
