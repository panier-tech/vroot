#ifndef TRACEE_MEMORY_H
#define TRACEE_MEMORY_H

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>

ssize_t tracee_read(
    pid_t pid,
    uint64_t remote_addr,
    void *local,
    size_t size
);

int tracee_read_string(
    pid_t pid,
    uint64_t remote_addr,
    char *buf,
    size_t size
);

#endif
