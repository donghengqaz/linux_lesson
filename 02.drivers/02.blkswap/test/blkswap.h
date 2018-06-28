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

#ifndef _BLKSWAP_H
#define _BLKSWAP_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct blkswap {
    int         fd;
    char        *pbuf;
    
    size_t      blks;
    size_t      len;

    size_t      send;
    size_t      recv;
};

/*
 * @brief open a block swap device
 * 
 * @param blks number of blocks
 * @param len length of blocks
 * 
 * @return NULL if failed or the device pointer
 */
struct blkswap *blkswap_open(size_t blks, size_t len);

/*
 * @brief recv a block which has data and bytes of block data
 * 
 * @param blkswap block swap device pointer
 * @param pblk block address pointer
 * @param size received bytes
 * 
 * @return 0 if success or others if failed
 */
int blkswap_recv(struct blkswap *blkswap, void **pblk, size_t *size);

/*
 * @brief get a free block swap which has no data
 * 
 * @param blkswap block swap device pointer
 * @param pblk block address pointer
 * 
 * @return 0 if success or others if failed
 */
int blkswap_get_blk(struct blkswap *blkswap, void **pblk);

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
int blkswap_send(struct blkswap *blkswap, size_t size);

/*
 * @brief finish read a block
 * 
 * @param blkswap block swap device pointer
 * @param size sent bytes
 * 
 * @return 0 if success or others if failed
 */
int blkswap_finish_read(struct blkswap *blkswap);

/*
 * @brief close a block swap device
 * 
 * @param blkswap block swap device pointer
 * 
 * @return 0 if success or others if failed
 */
int blkswap_close(struct blkswap *blkswap);

#ifdef __cplusplus
}
#endif

#endif /* _BLKSWAP_H */
