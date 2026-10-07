#define _GNU_SOURCE

#include "tracee.h"

#include <sys/uio.h>
#include <errno.h>
#include <string.h>

ssize_t tracee_read(
    pid_t pid,
    uint64_t remote_addr,
    void *local,
    size_t size
)
{
    struct iovec local_iov = {
        .iov_base = local,
        .iov_len = size,
    };

    struct iovec remote_iov = {
        .iov_base = (void *)remote_addr,
        .iov_len = size,
    };

    ssize_t n = process_vm_readv(
        pid,
        &local_iov,
        1,
        &remote_iov,
        1,
        0
    );

    if (n < 0)
        return -errno;

    return n;
}

int tracee_read_string(
    pid_t pid,
    uint64_t remote_addr,
    char *buf,
    size_t size
)
{
    if (size == 0)
        return -EINVAL;

    for (size_t i = 0; i < size; i++) {
        char c;

        ssize_t n = tracee_read(
            pid,
            remote_addr + i,
            &c,
            1
        );

        if (n != 1)
            return -EFAULT;

        buf[i] = c;

        if (c == '\0')
            return 0;
    }

    buf[size - 1] = '\0';
    return -ENAMETOOLONG;
}
