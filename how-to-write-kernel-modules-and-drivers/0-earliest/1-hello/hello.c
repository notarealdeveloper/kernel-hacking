#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

static int __init hello_init(void) {

    printk(KERN_DEBUG "Hello world!\n");
    return 0;
}

static void __exit hello_exit(void) {
    printk(KERN_DEBUG "Goodbye, cruel world!\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jason Wilkes <letshaveanadventure@gmail.com>");
MODULE_DESCRIPTION("A minimal kernel module");

