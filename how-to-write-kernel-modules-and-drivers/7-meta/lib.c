#include "util.h"

#include <linux/glob.h>
void lib_glob_usage(void)
{
	/* bool __pure glob_match(char const *pat, char const *str); */
	msg("glob_match(\"upca\",            \"cupcakes\") == %d", glob_match("upca", "cupcakes"));
	msg("glob_match(\"*upca*\",          \"cupcakes\") == %d", glob_match("*upca*", "cupcakes"));
	msg("glob_match(\"*u?ca*\",          \"cupcakes\") == %d", glob_match("*u?ca*", "cupcakes"));
	msg("glob_match(\"*u[penis]ca*\",    \"cupcakes\") == %d", glob_match("*u[penis]ca*", "cupcakes"));
	msg("glob_match(\"*upc[:alpha:]k*\", \"cupcakes\") == %d", glob_match("*upc[:alpha:]k*", "cupcakes"));
	msg("glob_match(\"*upc[:alpha:]k*\", \"cupc4kes\") == %d", glob_match("*upc[:alpha:]k*", "cupc4kes"));
}


#include <linux/ptrace.h>
#include <asm/syscall.h>
int lib_syscall_info(void)
{
	struct task_struct *target;
	struct pt_regs *regs;
	unsigned long sp;
	unsigned long pc;
	long callno;
	unsigned long args[6] = {};
	unsigned int  maxargs;
	int i;

	target = current;
	regs   = task_pt_regs(target);

	if (unlikely(!regs))
		return -EAGAIN;

	sp     = user_stack_pointer(regs);
	pc     = instruction_pointer(regs);
	callno = syscall_get_nr(target, regs);

	if (callno != -1L)
		syscall_get_arguments(target, regs, 0, maxargs = 6, args);

	msg("rsp            = %p", (void *)sp);
	msg("rip            = %p", (void *)pc);
	msg("target->comm   = %s", target->comm);
	msg("syscall number = %ld", callno);
	for (i = 0; i < 6; i++)
		msg("args[%d]        = %lu", i, args[i]);

	return 0;
}
