#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/kfifo.h>
#include <linux/poll.h>

#include "vhw.h"

#define KEY_MESG_MAX 128

#define CHARACTER_IRQ_BASE 5
#define CHARACTER_IRQ_MAX 4

struct character_dev {
    dev_t           devno;
    struct cdev     *cdev;
    struct class    *class;
    struct device   *device;
};

static struct character_dev s_character_dev;
static wait_queue_head_t s_wait_queue;
static DEFINE_KFIFO(key_fifo, uint8_t, KEY_MESG_MAX);

#define CONFIG_CHAR_DEBUG

#ifdef CONFIG_CHAR_DEBUG
#define CHAR_DEBUG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define CHAR_DEBUG(fmt, ...)
#endif

static inline bool is_block(struct file *pfile)
{
    return pfile->f_flags & O_NONBLOCK ? false : true;
}

static int character_dev_open(struct inode *pnode, struct file *pfile)
{
    init_waitqueue_head(&s_wait_queue);

    return 0;
}

static loff_t character_dev_llseek(struct file *pfile, loff_t off, int set)
{
    printk("character device testing module \"llseek\"\n");

    return 0;
}

static ssize_t character_dev_read(struct file *pfile, char __user *pbuf, size_t size, loff_t *off)
{
    int ret;
    uint8_t pdata[1];

    if (is_block(pfile)) {
        wait_event_interruptible(s_wait_queue, !kfifo_is_empty(&key_fifo));
    }
    ret = kfifo_out(&key_fifo, pdata, 1);

    if (ret)
        copy_to_user(pbuf, pdata, 1);

    return ret;
}

static ssize_t character_dev_write(struct file *pfile, const char __user *pbuf, size_t size, loff_t *off)
{
    int ret;
    char pdata[2];

    copy_from_user(pdata, pbuf, 2);
    CHAR_DEBUG("\"write\" data %d %d\n", pdata[0], pdata[1]);

    ret = vhw_set_gpio(pdata[0], pdata[1]);
    if (ret) {
        CHAR_DEBUG("vhw set gpio %d\n", ret);
        return ret;
    }

    return 2;
}

static ssize_t character_dev_read_iter(struct kiocb *kiocb, struct iov_iter *iov_iter)
{
    printk("character device testing module \"read_iter\"\n");

    return 0;
}

static ssize_t character_dev_write_iter(struct kiocb *kiocb, struct iov_iter *iov_iter)
{
    printk("character device testing module \"write_iter\"\n");

    return 0;
}

static int character_dev_iterate(struct file *pfile, struct dir_context *pdir)
{
    printk("character device testing module \"iterate\"\n");

    return 0;
}

static unsigned int character_dev_poll(struct file *pfile, struct poll_table_struct *poll_table)
{
    unsigned int mask = 0;

    poll_wait(pfile, &s_wait_queue, poll_table);

    if (!kfifo_is_empty(&key_fifo))
        mask |= POLLIN | POLLRDNORM;

    return mask;
}

static long character_dev_unlocked_ioctl(struct file *pfile, unsigned int cmd, unsigned long arg)
{
    printk("character device testing module \"unlocked_ioctl\"\n");

    return 0;
}

static long character_dev_compat_ioctl(struct file *pfile, unsigned int cmd, unsigned long arg)
{
    printk("character device testing module \"compat_ioctl\"\n");

    return 0;
}

static int character_dev_mmap(struct file *pfile, struct vm_area_struct *vm_area)
{
    printk("character device testing module \"mmap\"\n");

    return 0;    
}

static int character_dev_flush(struct file *pfile, fl_owner_t id)
{
    printk("character device testing module \"flush\"\n");

    return 0; 
}

static int character_dev_release(struct inode *pnode, struct file *pfile)
{
    printk("character device testing module \"release\"\n");

    return 0; 
}

static int character_dev_fsync(struct file *pfile, loff_t off1, loff_t off2, int datasync)
{
    printk("character device testing module \"fsync\"\n");

    return 0; 
}

static int character_dev_fasync(int index, struct file *pfile, int signal)
{
    printk("character device testing module \"fasync\"\n");

    return 0; 
}

static int character_dev_lock(struct file *pfile, int index, struct file_lock *lock)
{
    printk("character device testing module \"lock\"\n");

    return 0; 
}

static ssize_t character_dev_sendpage(struct file *pfile, struct page *page, int index, size_t size, loff_t *off, int own)
{
    printk("character device testing module \"sendpage\"\n");

    return 0; 
}

static unsigned long character_dev_get_unmapped_area(struct file *pfile, unsigned long arg1, unsigned long arg2, unsigned long arg3, unsigned long arg4)
{
    printk("character device testing module \"unmapped_area\"\n");

    return 0; 
}

static int character_dev_check_flags(int flag)
{
    printk("character device testing module \"flags\"\n");

    return 0; 
}

static int character_dev_flock(struct file *pfile, int index, struct file_lock *lock)
{
    printk("character device testing module \"flock\"\n");

    return 0; 
}

static ssize_t character_dev_splice_write(struct pipe_inode_info *pipe_inode, struct file *pfile, loff_t *off, size_t size, unsigned int index)
{
    printk("character device testing module \"splice_write\"\n");

    return 0; 
}

static ssize_t character_dev_splice_read(struct file *pfile, loff_t *off, struct pipe_inode_info *pipe_inode, size_t size, unsigned int index)
{
    printk("character device testing module \"splice_read\"\n");

    return 0; 
}

static int character_dev_setlease(struct file *pfile, long set, struct file_lock **lock, void **private)
{
    printk("character device testing module \"setlease\"\n");

    return 0; 
}

static long character_dev_fallocate(struct file *pfile, int mode, loff_t offset, loff_t len)
{
    printk("character device testing module \"fallocate\"\n");

    return 0; 
}

static void character_dev_show_fdinfo(struct seq_file *m, struct file *f)
{
    printk("character device testing module \"show_fdinfo\"\n");
}

static struct file_operations character_dev_fs = {
    .owner = THIS_MODULE,
    .open = character_dev_open,
    .llseek = character_dev_llseek,
    .read_iter = character_dev_read_iter,
    .write_iter = character_dev_write_iter,
    .iterate = character_dev_iterate,
    .read = character_dev_read,
    .write = character_dev_write,
    .unlocked_ioctl = character_dev_unlocked_ioctl,
    .compat_ioctl = character_dev_compat_ioctl,
    .mmap = character_dev_mmap,
    .flush = character_dev_flush,
    .fsync = character_dev_fsync,
    .fasync = character_dev_fasync,
    .lock = character_dev_lock,
    .sendpage = character_dev_sendpage,
    .get_unmapped_area = character_dev_get_unmapped_area,
    .check_flags = character_dev_check_flags,
    .flock = character_dev_flock,
    .splice_write = character_dev_splice_write,
    .splice_read = character_dev_splice_read,
    .setlease = character_dev_setlease,
    .fallocate = character_dev_fallocate,
    .show_fdinfo = character_dev_show_fdinfo,
    .poll = character_dev_poll,
    .release = character_dev_release,
};

static void character_dev_isr(int id, int val, void *arg)
{
    int ret;
    uint8_t key = id;

    ret = kfifo_in(&key_fifo, &key, sizeof(key));
    if (ret <= 0) {
        printk("in fifo error %d\n", ret);
    }
    wake_up(&s_wait_queue);

    printk("ID is %d, val is %d, arg is %p\n", id, val, arg);
}

__init static int character_dev_init(void)
{
    int i;
    int ret;
    dev_t devno;
    struct class *class;
    struct cdev *cdev;
    struct device *device;

    printk("character device testing module initialize start\n");

    for (i = CHARACTER_IRQ_BASE; i < CHARACTER_IRQ_BASE + CHARACTER_IRQ_MAX; i++) {
        ret = vhw_register_irq(i, character_dev_isr, NULL);
        if (ret)
            goto irq_fail;
    }

    cdev = kzalloc(sizeof(*cdev), GFP_KERNEL);
    if (!cdev)
        goto cdev_fail;

    ret = alloc_chrdev_region(&devno, 0, 1, "character");
    if (ret)
        goto devno_fail;

    cdev_init(cdev, &character_dev_fs);
    cdev->owner = THIS_MODULE;
    ret = cdev_add(cdev, devno, 1);
    if (ret)
        goto add_fail;

    class = class_create(THIS_MODULE, "character");
    if (!class)
        goto class_fail;
    device = device_create(class, NULL, devno, NULL, "character");
    if (!device)
        goto classdev_fail;

    s_character_dev.devno = devno;
    s_character_dev.cdev = cdev;
    s_character_dev.class = class;
    s_character_dev.device = device;

    printk("character device testing module initialize OK\n");

    return 0;

classdev_fail:
    printk("class device fail\n");
    class_destroy(class);
class_fail:
    printk("class fail\n");
    cdev_del(cdev);
add_fail:
    printk("add fail\n");
    unregister_chrdev_region(devno, 1); 
devno_fail:
    printk("devno fail\n");
    kfree(cdev);
cdev_fail:
    printk("cdev fail\n");
irq_fail:
    printk("IRQ fail\n");
    while (--i >= 0)
        vhw_unregister_irq(i);

    return -ENOMEM;
}

__exit static void character_dev_exit(void)
{
    int i;

    device_destroy(s_character_dev.class, s_character_dev.devno);
    class_destroy(s_character_dev.class);
    cdev_del(s_character_dev.cdev);
    unregister_chrdev_region(s_character_dev.devno, 1);
    kfree(s_character_dev.cdev);

    for (i = CHARACTER_IRQ_BASE; i < CHARACTER_IRQ_BASE + CHARACTER_IRQ_MAX; i++)
        vhw_unregister_irq(i);

    printk("character device testing module exit\n");
}

module_init(character_dev_init);
module_exit(character_dev_exit);
MODULE_LICENSE("GPL");
