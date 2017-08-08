#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

static void symbol_helloworld(void)
{
    printk("symbol module hello world\n");
}
EXPORT_SYMBOL(symbol_helloworld);

__init static int symbol_init(void)
{
    printk("symbol module initialize\n");

    return 0;
}

__exit static void symbol_exit(void)
{
    printk("symbol module exit\n");
}

module_init(symbol_init);
module_exit(symbol_exit);
