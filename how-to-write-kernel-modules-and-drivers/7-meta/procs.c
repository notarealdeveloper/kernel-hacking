#include <linux/sched.h>	/* for current */
#include "kapi.h"
#include "util.h"

void procs_print_task(struct task_struct *task)
{
	struct task_struct *parent;

	if (task == NULL)
		task = current;

	parent = task->parent;
	msg("current = %s (pid %u), parent = %s (pid %u)",
		task->comm, task->pid, parent->comm, parent->pid);
	procs_print_children(task);
}

void procs_print_children(struct task_struct *task)
{
	struct task_struct *child;

	if (list_empty(&task->children))
		return;

	list_for_each_entry(child, &task->children, sibling) {
		msg("\t\tchild %s (pid = %u)", child->comm, child->pid);
	}
}

/* more fun task_struct fields
 * ===========================
 * int on_cpu: 1 if the task is currently running on some cpu. 0 otherwise
 * volatile long state: -1 unrunnable, 0 runnable, >= 1 stopped
 * 	(checking this from module init shows state = 0 for insmod, which makes sense)
 * struct list_head tasks: the list head that we can use to traverse all processes.
 * char *comm: the name of the command that is running (e.g., gedit)
 * pid_t pid: process id
 * task_struct parent: the parent process
 * struct list_head children: list of children
 * struct list_head sibling: lets us access the parent's children list
 */
void procs_psaux(void)
{
	struct task_struct *task;
	for_each_process(task) {
		msg("%s (pid = %d)", task->comm, task->pid);
		procs_print_children(task);
	}
}

/* opens-up the procs_psaux function above
 * =======================================
 * Here are the relevant definitions
 * 
 * From include/linux/sched.h
 * --------------------------
 * #define next_task(p) 		list_entry_rcu((p)->tasks.next, struct task_struct, tasks)
 * #define for_each_process(p) 	for (p = &init_task; (p = next_task(p)) != &init_task; )
 * extern struct task_struct init_task;
 * 
 * From init/init_task.c
 * --------------------------
 * struct task_struct init_task = INIT_TASK(init_task);
 *
 * From include/linux/init_task.h
 * ------------------------------
 * INIT_TASK is defined here. It's a *big* macro.
 * To see its definition, use this command:
 * $ pcregrep -M 'define INIT_TASK\(tsk\)(.|\n)*?\n}' include/linux/init_task.h
 */

static void procs_psaux_v2(void)
{
	struct task_struct *task;
	for (task = &init_task; (task = next_task(task)) != &init_task; ) {
		msg("%s (pid = %d)", task->comm, task->pid);
	}
}

static void procs_psaux_v3(void)
{
	struct task_struct *task;
	/* Note: the list_head in any task_struct is called "tasks" */
	struct list_head *tasklist = &init_task.tasks;
	list_for_each_entry(task, tasklist, tasks) {
		msg("%s (pid = %d)", task->comm, task->pid);
	}
}

static void procs_psaux_v4(void)
{
	struct task_struct *task;
	list_for_each_entry(task, &init_task.tasks, tasks) {
		msg("%s (pid = %d)", task->comm, task->pid);
	}
}

static void procs_psaux_v5(void)
{
	struct task_struct *task;
	struct list_head   *pos;

	list_for_each(pos, &init_task.tasks) {
		task = list_entry(pos, struct task_struct, tasks);
		msg("%s (pid = %d)", task->comm, task->pid);
	}
}

