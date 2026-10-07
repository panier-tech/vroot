#ifndef VFS_PATH_H
#define VFS_PATH_H

#include <limits.h>
#include <stddef.h>


/*
 * Virtual filesystem context.
 *
 * rootfs:
 *     Host-side directory corresponding to guest "/".
 *
 * cwd:
 *     Current working directory in the guest namespace.
 *     Always an absolute, normalized guest path.
 */
struct vfs_context {
    char rootfs[PATH_MAX];
    char cwd[PATH_MAX];
};


/*
 * Result of path resolution.
 *
 * guest:
 *     Canonical absolute path in the guest namespace.
 *
 * host:
 *     Corresponding path in the host namespace.
 */
struct vfs_path {
    char guest[PATH_MAX];
    char host[PATH_MAX];
};


/*
 * Initialize a virtual filesystem context.
 *
 * rootfs may be either an absolute or relative host path.
 */
int vfs_init(
    struct vfs_context *ctx,
    const char *rootfs
);


/*
 * Change the guest current working directory.
 *
 * The path is interpreted in the guest namespace.
 */
int vfs_chdir(
    struct vfs_context *ctx,
    const char *path
);


/*
 * Resolve a guest pathname.
 *
 * The input may be absolute or relative.
 *
 * Example:
 *
 *     /etc/passwd
 *     ../etc/passwd
 *     ./foo
 */
int vfs_resolve(
    const struct vfs_context *ctx,
    const char *path,
    struct vfs_path *result
);


/*
 * Normalize an absolute guest path.
 *
 * This function does not access the filesystem.
 */
int vfs_normalize_guest_path(
    const char *path,
    char *output,
    size_t output_size
);

#endif
