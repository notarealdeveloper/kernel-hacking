#ifndef _KAPI_H
#define _KAPI_H

/* definitions that should be generally helpful */
#include "util.h"

/* includes for state.c */
#include "state.h"

/* includes for procs.c */
void procs_print_task(struct task_struct *task);
void procs_print_children(struct task_struct *task);
void procs_psaux(void);

/* includes for sysfs.c */
int  sysfs_enter(void);
void sysfs_leave(void);

/* includes for procfs.c */
int  procfs_init(void);
void procfs_exit(void);

/* includes for modules.c */
void modules_lsmod(void);

/* includes for uaccess.c */
void uaccess_access_ok(void *p);
void uaccess_show_access_info(void);
void uaccess_show_thread_info(void);

/* includes for lib.c */
void lib_glob_usage(void);
int  lib_syscall_info(void);

#endif /* _KAPI_H */
