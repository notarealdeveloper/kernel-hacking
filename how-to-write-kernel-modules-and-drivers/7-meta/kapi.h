#ifndef _KAPI_H
#define _KAPI_H

/* [*] Defines that should be generally helpful */
#define msg(s) 	printk(KERN_DEBUG "%s: " s "\n", __func__)

/* print whether we're in an interrupt() */
#include <linux/interrupt.h>	/* For in_interrupt() */
#define context_print() ({				\
	if (in_interrupt())				\
		msg("Running in interrupt context");	\
	else						\
		msg("Running in process context");	\
})

/* [*] process related stuff */
#include <linux/sched.h>	/* for current */
void procs_print_task(struct task_struct *task);
void procs_print_children(struct task_struct *task);
void procs_psaux(void);


/* [*] includes for sysfs.c */
int  kobj_init(void);
void kobj_kill(void);


/* [*] includes for modules.c */
void modules_lsmod(void);

/* [*] includes for state.c */
#include "state.h"

#endif /* _KAPI_H */
