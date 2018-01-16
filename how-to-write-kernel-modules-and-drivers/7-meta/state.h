#ifndef _STATE_H
#define _STATE_H

/* ignore the silly regs for now,
 * since this is just for fun */
struct regs {
	unsigned long 	rax;
	unsigned long 	rbx;
	unsigned long 	rcx;
	unsigned long 	rdx;

	unsigned long 	rsi;
	unsigned long 	rdi;
	unsigned long 	rsp;
	unsigned long 	rbp;

	unsigned long 	rip;

	unsigned long 	cr0;
};

#define cli()		asm("cli;\t\n" ::)
#define sti()		asm("sti;\t\n" ::)

#define setup_regs_for_test() 	\
({					\
	set_rax(0xaaaaaaaaaaaaaaaa);	\
	set_rbx(0xbbbbbbbbbbbbbbbb);	\
	set_rcx(0xcccccccccccccccc);	\
	set_rdx(0xdddddddddddddddd);	\
	set_rsi(0xeeeeeeeeeeeeeeee);	\
	set_rdi(0xffffffffffffffff);	\
})

#define state_save_regs(regs)		\
({					\
	register long rax asm("rax");	\
	register long rbx asm("rbx");	\
	register long rcx asm("rcx");	\
	register long rdx asm("rdx");	\
	register long rsi asm("rsi");	\
	register long rdi asm("rdi");	\
	register long rsp asm("rsp");	\
	register long rbp asm("rbp");	\
	(regs)->rip = get_rip(regs);	\
	(regs)->rax = rax;		\
	(regs)->rbx = rbx;		\
	(regs)->rcx = rcx;		\
	(regs)->rdx = rdx;		\
	(regs)->rdi = rdi;		\
	(regs)->rsi = rsi;		\
	(regs)->rsp = rsp;		\
	(regs)->rbp = rbp;		\
	(regs)->cr0 = get_cr0(regs);	\
})

#define get_rip(regs)			\
({					\
	asm volatile (			\
		"pushq %%rax\n\t"	\
		"call mylabel_%=\n\t"	\
		"mylabel_%=:\n\t"	\
		"popq %%rax\n\t"	\
		"movq %%rax, %0\n\t"	\
		"popq %%rax\n\t"	\
	:"=m"((regs)->rip)		\
	:				\
	:"memory"			\
	);				\
	(regs)->rip;			\
})

#define get_cr0(regs)			\
({					\
	asm volatile (			\
		"pushq %%rax\n\t"	\
		"movq %%cr0, %%rax\n\t"	\
		"movq %%rax, %0\n\t"	\
		"popq %%rax\n\t"	\
	:"=m"((regs)->cr0)		\
	:				\
	:"memory"			\
	);				\
	(regs)->cr0;			\
})

void state_dump(void);
void state_dump_regs(struct regs *regs);

void set_rax(unsigned long val);
void set_rbx(unsigned long val);
void set_rcx(unsigned long val);
void set_rdx(unsigned long val);
void set_rsi(unsigned long val);
void set_rdi(unsigned long val);

#endif /* _STATE_H */
