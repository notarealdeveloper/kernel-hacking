#include <asm/uaccess.h>
#include "util.h"

/* From arch/x86/include/asm/uaccess.h
 * #define user_addr_max() (current_thread_info()->addr_limit.seg)
 * #define KERNEL_DS	MAKE_MM_SEG(-1UL)
 * #define USER_DS 	MAKE_MM_SEG(TASK_SIZE_MAX)
 * #define get_ds()	(KERNEL_DS)
 * #define get_fs()	(current_thread_info()->addr_limit)
 */

void uaccess_access_ok(void *p)
{
	if (access_ok(VERIFY_READ, p, 1))
		msg("[+] RD access okay (%p)", p);
	else
		msg("[-] RD access fail (%p)", p);

	if (access_ok(VERIFY_WRITE, p, 1))
		msg("[+] WR access okay (%p)", p);
	else
		msg("[-] WR access fail (%p)", p);
}

void uaccess_show_access_info(void)
{
	uaccess_access_ok((void *)0x0000100000000000);
	uaccess_access_ok((void *)0x00007fffffff0000);
	uaccess_access_ok((void *)0x00007fffffffefff);
	uaccess_access_ok((void *)0x00007ffffffff000);
	uaccess_access_ok((void *)0x0000800000000000);
	uaccess_access_ok((void *)0xfffffffffffffff0);
}

void uaccess_show_thread_info(void)
{
	msg("user_addr_max() == %p", (void *)user_addr_max());
	msg("current_thread_info()->addr_limit.seg == %p", (void *)(current_thread_info()->addr_limit.seg));
	msg("current_thread_info()->cpu == %d",        current_thread_info()->cpu);
}

/* From arch/x86/include/asm/thread_info.h
struct thread_info {
	struct task_struct	*task;		// main task structure
	struct exec_domain	*exec_domain;	// execution domain
	__u32			flags;		// low level flags
	__u32			status;		// thread synchronous flags
	__u32			cpu;		// current CPU
	int			saved_preempt_count;
	mm_segment_t		addr_limit;
	struct restart_block    restart_block;
	void __user		*sysenter_return;
	unsigned int		sig_on_uaccess_error:1;
	unsigned int		uaccess_err:1;	// uaccess failed
};

DECLARE_PER_CPU(unsigned long, kernel_stack);

static inline struct thread_info *current_thread_info(void)
{
	struct thread_info *ti;
	ti = (void *)(this_cpu_read_stable(kernel_stack) + KERNEL_STACK_OFFSET - THREAD_SIZE);
	return ti;
}
*/
