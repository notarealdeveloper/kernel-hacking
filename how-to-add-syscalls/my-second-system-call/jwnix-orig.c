#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/sched.h>

#define msg(fmt, args...) \
	printk("In syscall %s: " fmt "\n", __func__, ## args);

SYSCALL_DEFINE0(jwnix)
{
	msg("Congratulations!");
	return 69;
}

SYSCALL_DEFINE1(waffle, unsigned long, arg)
{
	msg("arg == %lu. current: %s (%d)", arg, current->comm, current->pid);

	msg("Calling schedule()");
	schedule();
	msg("Back from schedule()");
	return 0;
}
