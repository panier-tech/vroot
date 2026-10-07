#define _GNU_SOURCE

#include "inject.h"

#include <linux/seccomp.h>
#include <sys/ioctl.h>
#include <errno.h>

int inject_fd(
    int listener_fd,
    uint64_t notification_id,
    int host_fd
)
{
    struct seccomp_notif_addfd addfd = {
        .id = notification_id,
        .flags = 0,
        .srcfd = host_fd,
        .newfd = 0,
        .newfd_flags = 0,
    };

    int ret = ioctl(
        listener_fd,
        SECCOMP_IOCTL_NOTIF_ADDFD,
        &addfd
    );

    if (ret < 0)
        return -errno;

    return ret;
}
