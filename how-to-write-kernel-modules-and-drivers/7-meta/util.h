#define msg(fmt, args...) 		\
	printk(KERN_DEBUG "%s: " fmt "\n", __func__, ## args)

/* Just for compile time. */
#define getcwd()			\
({					\
	static char *cwd = __FILE__;	\
	*strrchr(cwd, '/') = 0;		\
	cwd;				\
})

/*
static char *getcwd(void)
{
	static char *cwd = __FILE__;
	*(strrchr(cwd, '/') + 1) = 0;
	return cwd;
}
*/

