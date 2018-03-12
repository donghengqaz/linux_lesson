#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/time.h>

#if 0

int main(int argc, char *argv[])
{
    int ret;
    int fd;
    char key;
    bool led[4];

    fd = open("/dev/character", O_RDWR);
    if (fd < 0)
        goto open_fail;

    for (int i = 0; i < 4; i++)
        led[i] = false;

    while (1) {
        ret = read(fd, &key, sizeof(key));
        if (ret <= 0) {
            printf("read error %d\n", key);
        }
        
        if (key >= 5 && key <= 8) {
            char buf[2];
            int offset = key - 5;
            
            led[offset] = !led[offset];

            buf[0] = offset;
            buf[1] = led[offset];

            ret = write(fd, buf, sizeof(buf));
            if (ret <= 0) {
                printf("write error %d\n", key);
            }
        }
    }

    close(fd);

    return 0;

open_fail:
    printf("open fail %d\n", errno);
    return -1;
}

#else

int main(int argc, char *argv[])
{
    int ret;
    int fd;
    char key;
    bool led[4];

    fd = open("/dev/character", O_RDWR | O_NONBLOCK);
    if (fd < 0)
        goto open_fail;

    for (int i = 0; i < 4; i++)
        led[i] = false;

    while (1) {
        fd_set rfds;
        fd_set efds;

        struct timeval timeout = {
            .tv_sec = 10,
            .tv_usec = 0
        };

        FD_ZERO(&rfds);
        FD_ZERO(&efds);
        FD_SET(fd, &rfds);
        FD_SET(fd, &efds);

        ret = select(fd + 1, &rfds, NULL, &efds, &timeout);
        if (ret < 0) {
            printf("select error %d\n", errno);
            break;
        } else if (ret == 0) {
            printf("select timeout 10s\n");
            continue;
        }

        if (FD_ISSET(fd, &efds)) {
            printf("catch an error %d\n", errno);
            break;
        } else if (!FD_ISSET(fd, &rfds)) {
            printf("don't catch a reading event\n");
            continue;
        }

        ret = read(fd, &key, sizeof(key));
        if (ret <= 0) {
            printf("read error %d\n", key);
        }
        printf("Read Key %d\n", key);
        
        if (key >= 5 && key <= 8) {
            char buf[2];
            int offset = key - 5;
            
            led[offset] = !led[offset];

            buf[0] = offset;
            buf[1] = led[offset];

            ret = write(fd, buf, sizeof(buf));
            if (ret <= 0) {
                printf("write error %d\n", ret);
            } else {
                printf("set LED %d state %d\n", buf[0], led[offset]);
            }
        }
    }

    close(fd);

    return 0;

open_fail:
    printf("open fail %d\n", errno);
    return -1;
}

#endif
