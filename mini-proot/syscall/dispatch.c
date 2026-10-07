#include "syscall.h"
#include "handlers.h"

#include <sys/syscall.h>

struct syscall_result syscall_dispatch(
    struct syscall_request *req
)
{
    switch (req->nr) {

    case SYS_openat:
        return emulate_openat(req);

    default:
        return (struct syscall_result) {
            .action = SYSCALL_PASS,
        };
    }
}
