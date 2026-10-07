#ifndef FD_INJECT_H
#define FD_INJECT_H

#include <stdint.h>
#include <sys/types.h>

int inject_fd(
    int listener_fd,
    uint64_t notification_id,
    int host_fd
);

#endif
