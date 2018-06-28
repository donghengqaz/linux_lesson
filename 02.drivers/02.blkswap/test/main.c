

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "blkswap.h"

int main(void)
{
    pid_t id;
    struct blkswap *blkswap;
    
    blkswap = blkswap_open(10, 4096);
    if (!blkswap) {
        printf("error %d\n", __LINE__);
        return -1;
    }

    id = fork();
    if (id == 0) {
        int count = 5;

        while (count--) {
            int n;
            char *p = NULL;

            blkswap_get_blk(blkswap, (void **)&p);
            n = sprintf(p, "helloword %d", count);
            blkswap_send(blkswap, n);

            sleep(1);
        }
    } else {
        int count = 5;

        while (count--) {
            char *p = NULL;
            size_t bytes;

            blkswap_recv(blkswap, (void **)&p, &bytes);
            printf("get %s\n", p);
            blkswap_finish_read(blkswap);
        }        
    }
}
