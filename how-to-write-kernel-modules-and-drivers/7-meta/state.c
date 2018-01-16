#include <linux/printk.h>
#include "state.h"

/* define functions to let us set registers
 * ========================================
 * the clobber-list used to contain #reg, but gcc's inline assembly
 * is infinitely harder to reason about than *actual assembly*, so
 * it kept generating set_rax, set_rbx, etc. such that every one of
 * these functions was essentially identical, except for set_rbx, which
 * pushed rbx beforehand and popped it after, thus making the function
 * do exactly nothing. Removing rbx from the clobber list prevents gcc
 * from fucking with us like this, though it may eventually cause other problems.
 * 
 * I did a "test" where I wrapped kmod_init in some code that defined 13 local longs,
 * and then printed them after the function had finished everything else.
 * All the variables had the values they were supposed to have, so at very least
 * our asm fuckery isn't obviously changing them in a dangerous way.
 */
#define DEFINE_SETTER(reg) 		\
inline __attribute__((always_inline))	\
void set_##reg(unsigned long val) 	\
{					\
	asm volatile(			\
		"movq %0, %%" #reg ";\n"\
		::"m"(val) :		\
	);				\
}

DEFINE_SETTER(rax);
DEFINE_SETTER(rbx);
DEFINE_SETTER(rcx);
DEFINE_SETTER(rdx);
DEFINE_SETTER(rsi);
DEFINE_SETTER(rdi);

void state_dump(void)
{
	struct regs regs;
	state_save_regs(&regs);
	state_dump_regs(&regs);
	printk("%s: [*] Dumping stack. This is not an oops.\n", __func__);
	dump_stack();
}

void state_dump_regs(struct regs *regs)
{
	printk(KERN_INFO "%s: rax = %016lx\n", __func__, regs->rax);
	printk(KERN_INFO "%s: rbx = %016lx\n", __func__, regs->rbx);
	printk(KERN_INFO "%s: rcx = %016lx\n", __func__, regs->rcx);
	printk(KERN_INFO "%s: rdx = %016lx\n", __func__, regs->rdx);
	printk(KERN_INFO "%s: rsi = %016lx\n", __func__, regs->rsi);
	printk(KERN_INFO "%s: rdi = %016lx\n", __func__, regs->rdi);
	printk(KERN_INFO "%s: rsp = %016lx\n", __func__, regs->rsp);
	printk(KERN_INFO "%s: rbp = %016lx\n", __func__, regs->rbp);
	printk(KERN_INFO "%s: rip = %016lx\n", __func__, regs->rip);
	printk(KERN_INFO "%s: cr0 = %016lx\n", __func__, regs->cr0);
	// printk(KERN_INFO "rip = %016lx\n", __builtin_return_address(0));
}
