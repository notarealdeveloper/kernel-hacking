#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/sched.h>

#define msg(fmt, args...) \
	printk("In syscall %s: " fmt, __func__, ## args);

SYSCALL_DEFINE0(jwnix)
{
	printk("sys_jwnix: Congratulations!\n");
	return 69;
}

SYSCALL_DEFINE1(waffle, unsigned long, arg)
{
	printk("sys_waffle: arg == %lu. current: %s (pid %d)... ",
		arg, current->comm, current->pid);

	printk("Calling schedule()... ");
	schedule();
	printk("Back from schedule()\n");

	return 0;
}
