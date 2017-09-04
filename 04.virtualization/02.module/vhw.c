#include <linux/net.h>
#include <linux/inet.h>
#include <linux/in.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/kfifo.h>
#include <linux/semaphore.h>
#include <linux/module.h>

#include "vhw_def.h"

static struct task_struct *main_task, *event_task;
static struct socket *main_socket;
static struct sockaddr_in main_sockaddr;
static LIST_HEAD(irq_list);
static DEFINE_MUTEX(list_mutex);
static DEFINE_SEMAPHORE(report_sem);
static DEFINE_SEMAPHORE(sync_sem);
static DEFINE_KFIFO(data_fifo, struct vhw_event, VHW_FIFO_SIZE);

static int vhw_send_data(const void *buffer, int n)
{
    int ret;
    struct vhw_event event;

    if (!buffer || !n)
        return -EINVAL;

    if (!main_socket)
        return -ENOENT;

    event.pbuf = buffer;
    event.len = n;

    ret = kfifo_in(&data_fifo, &event, sizeof(event));
    if (ret <= 0) {
        printk("in fifo error %d\n", ret);
        return ret;
    }

    up(&report_sem);
    down_interruptible(&sync_sem);

    return 0;
}
EXPORT_SYMBOL(vhw_send_data);

static int vhw_set_gpio(int num, bool state)
{
    int gpio_event[3] = {GPIO_EVENT_ID, num, state};

    return vhw_send_data(gpio_event, sizeof(gpio_event));
}
EXPORT_SYMBOL(vhw_set_gpio);

int vhw_register_irq(int id, void (*func)(int id, int val, void *arg), void *arg)
{
    struct vhw_irq *peripheral;

    peripheral = kzalloc(sizeof(*peripheral), GFP_KERNEL);
    if (!peripheral)
        return -ENOMEM;

    peripheral->id = id;
    peripheral->func = func;
    peripheral->arg = arg;
    INIT_LIST_HEAD(&peripheral->list);

    mutex_lock(&list_mutex);
    list_add_tail(&peripheral->list, &irq_list);
    mutex_unlock(&list_mutex);

    return 0;
}
EXPORT_SYMBOL(vhw_register_irq);

void vhw_unregister_irq(int id)
{
    struct vhw_irq *peripheral, *n;

    mutex_lock(&list_mutex);
    list_for_each_entry_safe(peripheral, n, &irq_list, list) {
        if (peripheral->id == id) {
            list_del(&peripheral->list);
            kfree(peripheral);
            break;
        }
    }
    mutex_unlock(&list_mutex);
}
EXPORT_SYMBOL(vhw_unregister_irq);

static int vhw_main_entry(void *p)
{
    int ret;
    int loop;
    struct socket *socket;
    struct ip_mreq mreq;
    struct sockaddr_in sockaddr;

    ret = sock_create(PF_INET, SOCK_DGRAM, 0, &socket);
    if (ret)
        goto create_fail;
    main_socket = socket;

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = PF_INET;
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddr.sin_port = htons(VHW_UDP_PORT);
    ret = kernel_bind(socket, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    if (ret)
        goto bind_fail;

    loop = 0;
    ret = kernel_setsockopt(socket, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loop, sizeof(loop));
    if (ret)
        goto setopt_fail1;

    mreq.imr_multiaddr.s_addr = in_aton(VHW_GROUP);
    mreq.imr_interface.s_addr = htons(INADDR_ANY);
    ret = kernel_setsockopt(socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq));
    if (ret)
        goto setopt_fail2;

    //iov_len = count = sock_recvmsg.size ?
    while (1) {
        char buf[128];
        struct iovec iov[1] = {
            {
                .iov_base = buf,
                .iov_len = 127
            }
        };
        struct msghdr msg = {
            .msg_name = &main_sockaddr,
            .msg_namelen = sizeof(main_sockaddr),
            .msg_iter = {
                .type = ITER_IOVEC,
                .iov_offset = 0,
                .count = 127,
                .iov = iov,
                .nr_segs = 0,
            },
            .msg_control = NULL,
            .msg_controllen = 0,
            .msg_flags = 0
        };

        memset(buf, 0, 128);

        ret = sock_recvmsg(socket, &msg, 0);
        if (ret > 0) {
            int num, id, val;
            struct vhw_irq *peripheral;

            if (ret < 8) {
                printk("package length error\n");
                continue;
            }

            ret = sscanf(buf, "%d", &num);
            if (ret != 1) {
                printk("package payload error\n");
                continue;
            }

            id = num % 10000;
            val = num / 10000;

            if (id < 0 && id > VHW_IRQ_ID_MAX)
                continue;

            printk("receive message %s type is %d\n", buf, id);

            mutex_lock(&list_mutex);
            list_for_each_entry(peripheral, &irq_list, list) {
                if (peripheral->id == id) {
                    peripheral->func(id, val, peripheral->arg);
                    break;
                }
            }
            mutex_unlock(&list_mutex);
        } else if (ret <= 0) {
            printk("receive error %d\n", ret);
            break;
        }
    }

setopt_fail2:
    printk("enbale multicase error\n");
setopt_fail1:
    printk("disable multicase loop back error\n");
bind_fail:
    printk("release socket %p\n", socket);
    main_socket = NULL;
    sock_release(socket);
create_fail:
    printk("main thread exit\n");
    return -1;
}

static int vhw_send_udp(const void *p, int n)
{
    int ret;
    struct sockaddr_in sockaddr;
    struct iovec iov[1] = {
        {
            .iov_base = (void *)p,
            .iov_len = n
        }
    };
    struct msghdr msg = {
        .msg_name = &sockaddr,
        .msg_namelen = sizeof(main_sockaddr),
        .msg_iter = {
            .type = ITER_IOVEC,
            .iov_offset = 0,
            .count = n,
            .iov = iov,
            .nr_segs = 0,
        },
        .msg_control = NULL,
        .msg_controllen = 0,
        .msg_flags = 0,
        .msg_iocb = NULL
    };
    
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = PF_INET;
    sockaddr.sin_addr.s_addr = in_aton(VHW_GROUP);
    sockaddr.sin_port = htons(VHW_UDP_PORT);

    ret = sock_sendmsg(main_socket, &msg);
    if (ret <= 0) {
        printk("send message error %d, pbuf is %p, len is %lu, count %lu\n", ret, 
                            iov[0].iov_base, iov[0].iov_len, msg.msg_iter.count);
    }
    
    return 0;
}

static int vhw_event_task(void *p)
{
    while (1) {
        int ret;
        struct vhw_event event;
        unsigned int len;
        
        ret = down_interruptible(&report_sem);
        if (ret) {
            printk("down semaphore error %d\n", ret);
            break;
        }

        len = kfifo_out(&data_fifo, &event, sizeof(event));
        if (!len) {
            printk("no fifo data\n");
            break;
        }

        vhw_send_udp(event.pbuf, event.len);
        up(&sync_sem);
    }

    return 0;
}

__init static int vhw_init(void)
{
    down(&report_sem);
    down(&sync_sem);

    main_task = kthread_create(vhw_main_entry, NULL, "virtual_board%d", 1);
    if (!main_task)
        goto board_thread_fail;

    event_task = kthread_create(vhw_event_task, NULL, "put_board%d", 1);
    if (!event_task)
        goto put_thread_fail;

    wake_up_process(main_task);
    wake_up_process(event_task);

    printk("VHW initialize OK\n");

    return 0;

put_thread_fail:
    printk("event thread fail\n");
    kthread_stop(main_task);
    main_task = NULL;
board_thread_fail:
    printk("main thread fail\n");
    return -ENOMEM;
}

__exit static void vhw_deinit(void)
{
    if (main_socket) {
        printk("shutdown socket %p\n", main_socket);
        main_socket->ops->shutdown(main_socket, SHUT_RDWR);
    }

    printk("VHW deinitialize OK\n");
}

module_init(vhw_init);
module_exit(vhw_deinit);
MODULE_LICENSE("GPL");
