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
#include <linux/semaphore.h>

#define BLKSWAP_NAME "blkswap"

#define DEBUG(fmt, ...)

enum {
    BLKSWAP_INIT = 1000,
    BLKSWAP_RECV,
    BLKSWAP_FINISH,
    BLKSWAP_GET,
    BLKSWAP_SEND,
};

union blkswap_req {
    struct {
        size_t      blks;
        size_t      len;
    } init;

    struct {
        size_t      offset;
        size_t      bytes;
    } recv;

    struct {
        size_t      offset;
    } get;

    struct {
        size_t      size;
    } send;  
};

struct blkswap_dev {
    dev_t           devno;
    struct cdev     *cdev;
    struct class    *class;
    struct device   *device;

    size_t          blks;
    size_t          len;

    size_t          send;
    size_t          recv;

    char            *buf;

    wait_queue_head_t recv_queue;
    wait_queue_head_t send_queue;
};

static struct blkswap_dev s_blkswap_dev;

static inline bool is_block(struct file *pfile)
{
    return pfile->f_flags & O_NONBLOCK ? false : true;
}

static int blkswap_open(struct inode *pnode, struct file *pfile)
{
    init_waitqueue_head(&s_blkswap_dev.recv_queue);
    init_waitqueue_head(&s_blkswap_dev.send_queue);

    return 0;
}

static long blkswap_unlocked_ioctl(struct file *pfile, unsigned int cmd, unsigned long arg)
{
    void __user *argp = (void __user *)arg;
    union blkswap_req req;
    size_t send;

    if (copy_from_user(&req, argp, sizeof(union blkswap_req)))
		return -EFAULT;

    switch (cmd) {
        case BLKSWAP_INIT:
            s_blkswap_dev.blks = req.init.blks;
            s_blkswap_dev.len = req.init.len;
            s_blkswap_dev.send = s_blkswap_dev.recv = 0;
            DEBUG("init blocks %ld len %ld\n", req.init.blks, req.init.len);
            break;
        case BLKSWAP_RECV:
            wait_event_interruptible(s_blkswap_dev.recv_queue, s_blkswap_dev.recv != s_blkswap_dev.send);
            req.recv.offset = s_blkswap_dev.recv * s_blkswap_dev.len;
            DEBUG("recv blocks %ld offset %ld\n", s_blkswap_dev.recv, req.recv.offset);
            break;
        case BLKSWAP_FINISH:
            s_blkswap_dev.recv = (s_blkswap_dev.recv + 1) % s_blkswap_dev.len;
            wake_up_interruptible(&s_blkswap_dev.send_queue);
            DEBUG("finish blocks %ldn", s_blkswap_dev.recv - 1);
            break;
        case BLKSWAP_GET:
            send = (s_blkswap_dev.send + 1) % s_blkswap_dev.blks;
            wait_event_interruptible(s_blkswap_dev.send_queue, s_blkswap_dev.recv != send);
            req.get.offset = s_blkswap_dev.send * s_blkswap_dev.len;
            DEBUG("get blocks %ld offset %ld\n", s_blkswap_dev.send, req.get.offset);
            break;
        case BLKSWAP_SEND:
            s_blkswap_dev.send = (s_blkswap_dev.send + 1) % s_blkswap_dev.blks;
            wake_up_interruptible(&s_blkswap_dev.recv_queue);
            DEBUG("send blocks %ld\n", s_blkswap_dev.send - 1);
            break;
        default:
            return -EINVAL;
    }

    if (copy_to_user(argp, &req, sizeof(union blkswap_req)))
		return -EFAULT;

    return 0;
}

static int blkswap_mmap(struct file *pfile, struct vm_area_struct *vma)
{
    size_t size = vma->vm_end - vma->vm_start;

    DEBUG("mmap blocks size %ld\n", size);

    vma->vm_flags |= VM_IO;
    vma->vm_flags |= VM_NORESERVE;

    s_blkswap_dev.buf = kmalloc(size, GFP_KERNEL);
    if (!s_blkswap_dev.buf)
        return -ENOMEM;

    if (remap_pfn_range(vma, vma->vm_start, virt_to_phys(s_blkswap_dev.buf) >> 12, size, vma->vm_page_prot))
        return -EAGAIN;

    return 0;    
}

static int blkswap_release(struct inode *pnode, struct file *pfile)
{
    kfree(s_blkswap_dev.buf);
    s_blkswap_dev.buf = NULL;

    return 0; 
}

static struct file_operations blkswap_fs = {
    .owner = THIS_MODULE,
    .open = blkswap_open,
    .unlocked_ioctl = blkswap_unlocked_ioctl,
    .mmap = blkswap_mmap,
    .release = blkswap_release,
};

__init static int blkswap_init(void)
{
    int ret;
    dev_t devno;
    struct class *class;
    struct cdev *cdev;
    struct device *device;

    DEBUG("block swap device testing module initialize start\n");

    cdev = kzalloc(sizeof(*cdev), GFP_KERNEL);
    if (!cdev)
        goto cdev_fail;

    ret = alloc_chrdev_region(&devno, 0, 1, BLKSWAP_NAME);
    if (ret)
        goto devno_fail;

    cdev_init(cdev, &blkswap_fs);
    cdev->owner = THIS_MODULE;
    ret = cdev_add(cdev, devno, 1);
    if (ret)
        goto add_fail;

    class = class_create(THIS_MODULE, BLKSWAP_NAME);
    if (!class)
        goto class_fail;
    device = device_create(class, NULL, devno, NULL, BLKSWAP_NAME);
    if (!device)
        goto classdev_fail;

    s_blkswap_dev.devno = devno;
    s_blkswap_dev.cdev = cdev;
    s_blkswap_dev.class = class;
    s_blkswap_dev.device = device;

    DEBUG("block swap device testing module initialize OK\n");

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

    return -ENOMEM;
}

__exit static void blkswap_exit(void)
{
    device_destroy(s_blkswap_dev.class, s_blkswap_dev.devno);
    class_destroy(s_blkswap_dev.class);
    cdev_del(s_blkswap_dev.cdev);
    unregister_chrdev_region(s_blkswap_dev.devno, 1);
    kfree(s_blkswap_dev.cdev);

    DEBUG("block swap device testing module exit\n");
}

module_init(blkswap_init);
module_exit(blkswap_exit);
MODULE_LICENSE("GPL");
