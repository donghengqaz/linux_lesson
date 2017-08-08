#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

__init static int helloworld_init(void)
{
    extern void symbol_helloworld(void);

    printk("hello world module initialize\n");

    symbol_helloworld();

    return 0;
}

__exit static void helloworld_exit(void)
{
    printk("hello world module exit\n");
}

module_init(helloworld_init);
module_exit(helloworld_exit);
