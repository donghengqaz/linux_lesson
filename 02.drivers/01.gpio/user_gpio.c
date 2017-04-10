#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/gpio/driver.h>

#define USER_GPIO_BASE 0
#define USER_GPIO_NUM 100

static struct gpio_chip *chip;

int user_gpio_request(struct gpio_chip *chip, unsigned offset)
{
    printk("user request %s GPIO %d\n", chip->label, offset);

    return 0;
}

void user_gpio_free(struct gpio_chip *chip, unsigned offset)
{
    printk("user free %s GPIO %d\n", chip->label, offset);
}

int user_gpio_dir_in(struct gpio_chip *chip, unsigned offset)
{
    printk("user set direction in %s GPIO %d\n", chip->label, offset);

    return 0;
}

int user_gpio_dir_out(struct gpio_chip *chip, unsigned offset, int value)
{
    printk("user set direction out %s GPIO %d\n", chip->label, offset);

    return 0;
}

int user_gpio_get_value(struct gpio_chip *chip, unsigned offset)
{
    printk("user get value in %s GPIO %d\n", chip->label, offset);

    return 0;
}

void user_gpio_set_value(struct gpio_chip *chip, unsigned offset, int value)
{
    printk("user set value in %s GPIO %d to %d\n", chip->label, offset, value);
}

__init static int user_gpio_init(void)
{
    int ret;

    printk("user GPIO module initialize starts ...\n");

    chip = kzalloc(sizeof(*chip), GFP_KERNEL);
    if (!chip)
        return -ENOMEM;

    chip->base = USER_GPIO_BASE;
    chip->ngpio = USER_GPIO_NUM;
    chip->label = "user_gpio";
    chip->owner = THIS_MODULE;

    chip->request = user_gpio_request;
    chip->free = user_gpio_free;
    chip->direction_input = user_gpio_dir_in;
    chip->direction_output = user_gpio_dir_out;
    chip->get = user_gpio_get_value;
    chip->set = user_gpio_set_value;

    ret = gpiochip_add(chip);
    if (ret) {
        printk("user add GPIO to chip error %d\n", ret);
        goto chip_err;
    }

    printk("user GPIO module initialize OK\n");

    return 0;

chip_err:
    kfree(chip);
    return ret;
}

__exit static void user_gpio_exit(void)
{
    gpiochip_remove(chip);

    printk("user GPIO module exit\n");
}

module_init(user_gpio_init);
module_exit(user_gpio_exit);

MODULE_LICENSE("GPL");
