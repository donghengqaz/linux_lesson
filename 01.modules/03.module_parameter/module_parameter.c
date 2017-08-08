#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

static int int_val = 0;
static char *str_val = "null";

module_param(int_val, int, S_IRUGO);
module_param(str_val, charp, S_IRUGO);

__init static int module_parameter_init(void)
{
    printk("module parameter testing module initialize\n");
    printk("parameter1 \"int_val\" is %d\n", int_val);
    printk("parameter2 \"str_val\" is %s\n", str_val);

    return 0;
}

__exit static void module_parameter_exit(void)
{
    printk("parameter1 \"int_val\" is %d\n", int_val);
    printk("parameter2 \"str_val\" is %s\n", str_val);
    printk("module parameter testing module exit\n");
}

module_init(module_parameter_init);
module_exit(module_parameter_exit);
