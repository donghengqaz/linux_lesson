// MIT License

// Copyright (c) 2017 dongheng

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <aio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/errno.h>
#include "blkswap.h"

#define BLKSWAP_NAME "blkswap"

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

/*
 * @brief open a block swap device
 */
struct blkswap *blkswap_open(size_t blks, size_t len)
{
    int fd, ret;
    char *pbuf;
    struct blkswap *blkswap;
    size_t size = blks * len;
    union blkswap_req req;

    fd = open("/dev/" BLKSWAP_NAME, O_RDWR);
    if (fd < 0)
        goto open_err;

    req.init.blks = blks;
    req.init.len = len;
    ret = ioctl(fd, BLKSWAP_INIT, &req);
    if (fd < 0)
        goto mmap_err;

    pbuf = (char *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (!pbuf)
        goto mmap_err;

    blkswap = (struct blkswap *)malloc(sizeof(struct blkswap));
    if (!blkswap)
        goto malloc_err;

    blkswap->fd = fd;
    blkswap->pbuf = pbuf;
    
    blkswap->blks = blks;
    blkswap->len = len;

    return blkswap;

malloc_err:
    munmap(pbuf, size);
mmap_err:
    close(fd);
open_err:
    return NULL;
}

/*
 * @brief recv a block which has data and bytes of block data
 */
int blkswap_recv(struct blkswap *blkswap, void **pblk, size_t *size)
{
    int ret;
    union blkswap_req req;

    ret = ioctl(blkswap->fd, BLKSWAP_RECV, &req);
    if (ret)
        return -1;

    *pblk = blkswap->pbuf + req.recv.offset;
    *size = req.recv.bytes;

    return ret;
}

/*
 * @brief get a free block swap which has no data
 * 
 * @param blkswap block swap device pointer
 * @param pblk block address pointer
 * 
 * @return 0 if success or others if failed
 */
int blkswap_get_blk(struct blkswap *blkswap, void **pblk)
{
    int ret;
    union blkswap_req req;

    ret = ioctl(blkswap->fd, BLKSWAP_GET, &req);
    if (ret)
        return -1;

    *pblk = blkswap->pbuf + req.get.offset;

    return 0;
}

/*
 * @brief send a block
 * 
 * @param blkswap block swap device pointer
 * @param size sent bytes
 * 
 * @return 0 if success or others if failed
 * 
 * @note the sent is that "blkswap_mmap" outputs
 */
int blkswap_send(struct blkswap *blkswap, size_t size)
{
    int ret;
    union blkswap_req req;

    req.send.size = size;
    ret = ioctl(blkswap->fd, BLKSWAP_SEND, &req);

    return ret;
}

/*
 * @brief finish read a block
 * 
 * @param blkswap block swap device pointer
 * @param size sent bytes
 * 
 * @return 0 if success or others if failed
 */
int blkswap_finish_read(struct blkswap *blkswap)
{
    int ret;
    union blkswap_req req;

    req.send.size = 0;
    ret = ioctl(blkswap->fd, BLKSWAP_FINISH, &req);

    return ret;
}

/*
 * @brief close a block swap device
 */
int blkswap_close(struct blkswap *blkswap)
{
    int ret;

    ret = munmap(blkswap->pbuf, blkswap->blks * blkswap->len);
    if (ret)
        return ret;

    ret = close(blkswap->fd);
    if (ret)
        return ret;

    free(blkswap);

    return 0;
}
