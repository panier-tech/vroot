#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include <sys/types.h>

enum syscall_action {
    SYSCALL_PASS,
    SYSCALL_EMULATE,
};

struct syscall_request {
    int listener_fd;

    uint64_t id;
    pid_t pid;

    int nr;
    uint64_t args[6];
};

struct syscall_result {
    enum syscall_action action;

    /*
     * 成功:
     *     value = syscall return value
     *     error = 0
     *
     * 失敗:
     *     value = -1
     *     error = errno
     */
    long value;
    int error;
};

struct syscall_result syscall_dispatch(
    struct syscall_request *req
);

#endif
