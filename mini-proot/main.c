#define _GNU_SOURCE

#include "syscall/syscall.h"
#include "syscall/handlers.h"

#include <linux/seccomp.h>

#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/syscall.h>

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int recv_notification(
    int listener_fd,
    struct seccomp_notif *req
)
{
    memset(req, 0, sizeof(*req));

    if (ioctl(
        listener_fd,
        SECCOMP_IOCTL_NOTIF_RECV,
        req
    ) < 0) {
        return -errno;
    }

    return 0;
}

static int send_response(
    int listener_fd,
    uint64_t id,
    struct syscall_result *result
)
{
    struct seccomp_notif_resp resp;

    memset(&resp, 0, sizeof(resp));

    resp.id = id;

    if (result->action == SYSCALL_PASS) {
        /*
         * syscall を tracee 自身に実行させる。
         */
        resp.flags = SECCOMP_USER_NOTIF_FLAG_CONTINUE;
        resp.val = 0;
        resp.error = 0;
    }
    else {
        /*
         * supervisor が syscall の結果を返す。
         */
        resp.flags = 0;
        resp.val = result->value;

        if (result->error != 0)
            resp.error = -result->error;
        else
            resp.error = 0;
    }

    if (ioctl(
        listener_fd,
        SECCOMP_IOCTL_NOTIF_SEND,
        &resp
    ) < 0) {
        return -errno;
    }

    return 0;
}

static void supervisor_loop(
    int listener_fd
)
{
    printf(
        "[supervisor] listener fd = %d\n",
        listener_fd
    );

    for (;;) {

        struct seccomp_notif notif;

        int ret = recv_notification(
            listener_fd,
            &notif
        );

        if (ret < 0) {
            if (-ret == EINTR)
                continue;

            fprintf(
                stderr,
                "[supervisor] NOTIF_RECV: %s\n",
                strerror(-ret)
            );

            break;
        }

        struct syscall_request req;

        memset(&req, 0, sizeof(req));

        req.listener_fd = listener_fd;
        req.id = notif.id;
        req.pid = notif.pid;

        req.nr = notif.data.nr;

        for (int i = 0; i < 6; i++)
            req.args[i] = notif.data.args[i];

        printf(
            "[supervisor] pid=%d syscall=%d\n",
            req.pid,
            req.nr
        );

        struct syscall_result result =
            syscall_dispatch(&req);

        ret = send_response(
            listener_fd,
            notif.id,
            &result
        );

        if (ret < 0) {
            fprintf(
                stderr,
                "[supervisor] NOTIF_SEND: %s\n",
                strerror(-ret)
            );

            break;
        }
    }
}
