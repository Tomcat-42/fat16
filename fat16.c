#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pablo Alessandro Santos Hugen <pablohuggem@gmail.com>");
MODULE_DESCRIPTION("Linux kernel module skeleton");

static int __init helloworld_init(void)
{
	pr_info("Hello world initialization!\n");

	return 0;
}

static void __exit helloworld_exit(void)
{
	pr_info("Hello world exit\n");
}

module_init(helloworld_init);
module_exit(helloworld_exit);
