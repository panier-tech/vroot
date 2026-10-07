#ifndef SYSCALL_HANDLERS_H
#define SYSCALL_HANDLERS_H

#include "syscall.h"
#include "../include/path.h"

void syscall_handlers_init(struct vfs_context *ctx);

struct syscall_result emulate_openat(
    struct syscall_request *req
);

#endif
