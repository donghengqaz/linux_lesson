#ifndef _VHW_DEF_H_
#define _VHW_DEF_H_

#include <linux/types.h>

/* virtual hardware UDP port */
#define VHW_UDP_PORT            14212
/* virtual hardware UDP Multicast address */
#define VHW_GROUP "224.0.2.66"
/* virtual FIFO size */
#define VHW_FIFO_SIZE           128

enum {
    GPIO_EVENT_ID = 1,  /* virtual hardware maximum IRQ ID  */

    VHW_IRQ_ID_MAX = 100 /* virtual hardware maximum IRQ ID */
};

struct vhw_irq {
    struct list_head        list;
    int                     id;
    void                    *arg;
    void (*func)(int id, int val, void *arg);
};

struct vhw_event {
    const char              *pbuf;
    uint32_t                len;
};

#endif /* _VHW_DEF_H_ */
