#ifndef _UTIL_H
#define _UTIL_H

#include <linux/printk.h>

#define msg(fmt, args...) 		\
	printk(KERN_DEBUG "%s: " fmt "\n", __func__, ## args)


/* Just for compile time. */
#define getcwd()			\
({					\
	static char *cwd = __FILE__;	\
	*strrchr(cwd, '/') = 0;		\
	cwd;				\
})


/* print whether we're in an interrupt() */
#include <linux/interrupt.h>	/* For in_interrupt() */
#define context_print() ({				\
	if (in_interrupt())				\
		msg("Running in interrupt context");	\
	else						\
		msg("Running in process context");	\
})

#endif /* _UTIL_H */
