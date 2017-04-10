#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <unistd.h>

/**
 * write the value of the gpio using command with the format "./gpio |offset| |state|"
 * for example "./gpio 10 h" or "./gpio 10 l".
 *
 * read the value of the gpio using command with the format "./gpio |offset| s"
 * for example "./gpio 10 s".
 */

#define CMD_ARGC_MAX 3

#define GPIO_EXPORT_DIR    "/sys/class/gpio/"
#define GPIO_EXPORT_FILE   GPIO_EXPORT_DIR "export"
#define GPIO_UNEXPORT_FILE GPIO_EXPORT_DIR "unexport"

typedef enum gpio_cmd {
    GPIO_SET_HIGH = 0,
    GPIO_SET_LOW,
    GPIO_READ_VALUE,

    GPIO_CMD_MAX
} gpio_cmd_t;

static gpio_cmd_t get_gpio_cmd(const char *s)
{
    int i;
    static const char *cmd[GPIO_CMD_MAX] = {
        "h",
        "l",
        "s"
    };

    for (i = 0; i < GPIO_CMD_MAX; i++) {
        if (strcmp(cmd[i], s) == 0)
            break;
    }

    return (gpio_cmd_t)i;
}

static int _gpio_offset(unsigned offset, const char *file)
{
    int ret;
    int fd;
    char buf[32];

    fd = open(file, O_WRONLY);
    if (fd < 0) {
        printf("open file [%s] failed [%d]\n", file, errno);
        return -EINVAL;
    }

    sprintf(buf, "%d", offset);

    ret = write(fd, buf, strlen(buf));
    if (ret <= 0) {
        printf("write [%s] to file [%s] failed [%d]\n", buf, file, errno);
    } else {
        ret = 0;
    }

    close(fd);

    return ret;
}

#define REQUST_GPIO(off) _gpio_offset(off, GPIO_EXPORT_FILE)
#define FREE_GPIO(off) _gpio_offset(off, GPIO_UNEXPORT_FILE)

static int goio_direction(unsigned offset, int d)
{
    int ret;
    int fd;
    char buf[8] = "";
    char file[128];

    sprintf(file, GPIO_EXPORT_DIR "gpio%d/direction",offset);
    fd = open(file, O_WRONLY);
    if (fd < 0) {
        printf("open file [%s] failed [%d]\n", file, errno);
        return -EINVAL;
    }

    if (!d) {
        if (strcmp(buf, "in"))
            sprintf(buf, "in");
    } else {
        if (strcmp(buf, "out"))
            sprintf(buf, "out");        
    }

    ret = write(fd, buf, strlen(buf));
    if (ret < 0) {
        printf("set [%d] gpio direction error %d\n", offset, errno);
    } else {
        ret = 0;
    }

    close(fd);

    return 0;      
}

#define SET_GPIO_INPUT(off) goio_direction(off, 0)
#define SET_GPIO_OUTPUT(off) goio_direction(off, 1)

static int gpio_set_value(unsigned offset, const char *value)
{
    int ret;
    int fd;
    char file[128];

    SET_GPIO_OUTPUT(offset);

    sprintf(file, GPIO_EXPORT_DIR "gpio%d/value",offset); 
    fd = open(file, O_WRONLY);
    if (fd < 0) {
        printf("open file [%s] failed [%d]\n", file, errno);
        return -EINVAL;
    }

    ret = write(fd, value, strlen(value));
    if (ret < 0) {
        printf("write [%s] to file [%s] failed [%d]\n", value, file, errno);
    } else {
        printf("set [%d] gpio to be %s\n", offset, value);
        ret = 0;
    }

    close(fd);

    return ret;
}

#define SET_GPIO_LOW(off) gpio_set_value(off, "0")
#define SET_GPIO_HIGH(off) gpio_set_value(off, "1")

static int gpio_read_state(unsigned offset)
{
    int ret;
    int fd;
    char buf[8];
    char file[128];

    SET_GPIO_INPUT(offset);

    sprintf(file, GPIO_EXPORT_DIR "gpio%d/value",offset); 
    fd = open(file, O_RDONLY);
    if (fd < 0) {
        printf("open file [%s] failed [%d]\n", file, errno);
        return -EINVAL;
    }

    ret = read(fd, buf, sizeof(buf));
    if (ret < 0) {
        printf("read file [%s] failed [%d]\n", file, errno);
    } else {
        printf("gpio state is %s\n", buf);
        ret = 0;
    }

    close(fd);

    return ret;
}

int main(int argc, char **argv)
{
    int ret;
    int fd;
    unsigned offset;
    gpio_cmd_t cmd;

    if (argc > CMD_ARGC_MAX) {
        printf("input argc is %d\n", argc);
        return -EINVAL;
    }

    cmd = get_gpio_cmd(argv[2]);
    if (cmd >= GPIO_CMD_MAX) {
        printf("input command error\n");
        goto cmd_err;
    }

    offset = atoi(argv[1]);
    ret = REQUST_GPIO(offset);
    if (ret)
        goto offset_err;

    switch (cmd) {
        case GPIO_SET_HIGH:
            ret = SET_GPIO_LOW(offset);
            break;
        case GPIO_SET_LOW:
            ret = SET_GPIO_HIGH(offset);
            break;
        case GPIO_READ_VALUE:
            ret = gpio_read_state(offset);
            break;
        default:
            break;
    }

    FREE_GPIO(offset);

    return ret;

offset_err:
    FREE_GPIO(offset);
cmd_err:
    return ret;
}