#define _GNU_SOURCE

#include "handlers.h"

#include "../memory/tracee.h"
#include "../fd/inject.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/syscall.h>

static struct vfs_context *vfs;

void syscall_handlers_init(struct vfs_context *ctx)
{
    vfs = ctx;
}

struct syscall_result emulate_openat(
    struct syscall_request *req
)
{
    int dirfd = (int)req->args[0];
    uint64_t pathname_ptr = req->args[1];
    int flags = (int)req->args[2];
    mode_t mode = (mode_t)req->args[3];

    /*
     * 第1段階では AT_FDCWD のみ。
     */
    if (dirfd != AT_FDCWD) {
        return (struct syscall_result) {
            .action = SYSCALL_PASS,
        };
    }

    char guest_path[PATH_MAX];

    int ret = tracee_read_string(
        req->pid,
        pathname_ptr,
        guest_path,
        sizeof(guest_path)
    );

    if (ret < 0) {
        return (struct syscall_result) {
            .action = SYSCALL_EMULATE,
            .value = -1,
            .error = -ret,
        };
    }

    struct vfs_path path;

    ret = vfs_resolve(
        vfs,
        guest_path,
        &path
    );

    if (ret < 0) {
        return (struct syscall_result) {
            .action = SYSCALL_EMULATE,
            .value = -1,
            .error = -ret,
        };
    }

    int host_fd = open(
        path.host,
        flags,
        mode
    );

    if (host_fd < 0) {
        return (struct syscall_result) {
            .action = SYSCALL_EMULATE,
            .value = -1,
            .error = errno,
        };
    }

    int guest_fd = inject_fd(
        req->listener_fd,
        req->id,
        host_fd
    );

    close(host_fd);

    if (guest_fd < 0) {
        return (struct syscall_result) {
            .action = SYSCALL_EMULATE,
            .value = -1,
            .error = -guest_fd,
        };
    }

    return (struct syscall_result) {
        .action = SYSCALL_EMULATE,
        .value = guest_fd,
        .error = 0,
    };
}
