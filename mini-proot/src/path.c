#define _GNU_SOURCE

#include "path.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/*
 * Normalize an absolute guest path.
 *
 * Examples:
 *
 *     /                       -> /
 *     /a                      -> /a
 *     /a/./b                 -> /a/b
 *     /a/b/../c              -> /a/c
 *     /../../etc/passwd      -> /etc/passwd
 *
 * This function performs purely lexical normalization.
 *
 * It does NOT access the filesystem.
 */
int vfs_normalize_guest_path(
    const char *path,
    char *output,
    size_t output_size
)
{
    if (!path || !output || output_size == 0) {
        errno = EINVAL;
        return -1;
    }

    /*
     * This function only accepts absolute paths.
     */
    if (path[0] != '/') {
        errno = EINVAL;
        return -1;
    }

    /*
     * strtok_r() modifies its input.
     */
    char *tmp = strdup(path);

    if (!tmp) {
        return -1;
    }

    /*
     * Maximum number of path components.
     *
     * PATH_MAX is sufficient for our current implementation.
     */
    size_t max_components = PATH_MAX / 2 + 1;

    char **components = calloc(
        max_components,
        sizeof(*components)
    );

    if (!components) {
        free(tmp);
        return -1;
    }

    size_t count = 0;

    char *saveptr = NULL;

    char *token = strtok_r(
        tmp,
        "/",
        &saveptr
    );

    while (token) {

        /*
         * "." means current directory.
         */
        if (strcmp(token, ".") == 0) {
            /*
             * Nothing to do.
             */
        }

        /*
         * ".." means parent directory.
         */
        else if (strcmp(token, "..") == 0) {

            /*
             * Do not allow the guest path to escape
             * above the virtual root.
             */
            if (count > 0) {
                count--;
            }
        }

        /*
         * Normal component.
         */
        else {

            if (count >= max_components) {
                free(components);
                free(tmp);

                errno = ENAMETOOLONG;
                return -1;
            }

            components[count++] = token;
        }

        token = strtok_r(
            NULL,
            "/",
            &saveptr
        );
    }

    /*
     * Construct normalized path.
     */
    size_t pos = 0;

    if (output_size < 2) {
        free(components);
        free(tmp);

        errno = ENAMETOOLONG;
        return -1;
    }

    output[pos++] = '/';

    for (size_t i = 0; i < count; i++) {

        size_t len = strlen(components[i]);

        if (pos + len >= output_size) {
            free(components);
            free(tmp);

            errno = ENAMETOOLONG;
            return -1;
        }

        memcpy(
            output + pos,
            components[i],
            len
        );

        pos += len;

        if (i + 1 < count) {

            if (pos + 1 >= output_size) {
                free(components);
                free(tmp);

                errno = ENAMETOOLONG;
                return -1;
            }

            output[pos++] = '/';
        }
    }

    output[pos] = '\0';

    free(components);
    free(tmp);

    return 0;
}


/*
 * Convert a guest pathname to an absolute guest pathname.
 *
 * If path is absolute:
 *
 *     /etc/passwd
 *         -> /etc/passwd
 *
 * If path is relative:
 *
 *     cwd = /home/user
 *     path = ../etc/passwd
 *
 *         -> /home/etc/passwd
 *
 * This function does NOT normalize "." or "..".
 * Normalization is performed separately.
 */
static int make_absolute_guest_path(
    const struct vfs_context *ctx,
    const char *path,
    char *output,
    size_t output_size
)
{
    if (!ctx || !path || !output || output_size == 0) {
        errno = EINVAL;
        return -1;
    }

    /*
     * Absolute guest path.
     */
    if (path[0] == '/') {

        size_t len = strlen(path);

        if (len >= output_size) {
            errno = ENAMETOOLONG;
            return -1;
        }

        memcpy(
            output,
            path,
            len + 1
        );

        return 0;
    }

    /*
     * Relative guest path.
     */
    int ret = snprintf(
        output,
        output_size,
        "%s/%s",
        ctx->cwd,
        path
    );

    if (ret < 0) {
        errno = EINVAL;
        return -1;
    }

    if ((size_t)ret >= output_size) {
        errno = ENAMETOOLONG;
        return -1;
    }

    return 0;
}


/*
 * Convert the configured rootfs into an absolute host path.
 *
 * We deliberately do NOT use realpath().
 *
 * realpath() requires the target path to exist.
 * The VFS layer should not have that restriction.
 */
static int make_absolute_rootfs(
    const char *rootfs,
    char *output,
    size_t output_size
)
{
    if (!rootfs || !output || output_size == 0) {
        errno = EINVAL;
        return -1;
    }

    /*
     * Already absolute.
     */
    if (rootfs[0] == '/') {

        size_t len = strlen(rootfs);

        if (len >= output_size) {
            errno = ENAMETOOLONG;
            return -1;
        }

        memcpy(
            output,
            rootfs,
            len + 1
        );

        return 0;
    }

    /*
     * Relative rootfs.
     */
    char cwd[PATH_MAX];

    if (!getcwd(cwd, sizeof(cwd))) {
        return -1;
    }

    int ret = snprintf(
        output,
        output_size,
        "%s/%s",
        cwd,
        rootfs
    );

    if (ret < 0) {
        errno = EINVAL;
        return -1;
    }

    if ((size_t)ret >= output_size) {
        errno = ENAMETOOLONG;
        return -1;
    }

    return 0;
}


/*
 * Initialize the VFS context.
 */
int vfs_init(
    struct vfs_context *ctx,
    const char *rootfs
)
{
    if (!ctx || !rootfs || rootfs[0] == '\0') {
        errno = EINVAL;
        return -1;
    }

    /*
     * rootfs itself must exist.
     */
    if (access(rootfs, F_OK) < 0) {
        return -1;
    }

    /*
     * Store an absolute host-side rootfs path.
     */
    if (make_absolute_rootfs(
            rootfs,
            ctx->rootfs,
            sizeof(ctx->rootfs)) < 0) {
        return -1;
    }

    /*
     * Guest starts at "/".
     */
    strcpy(ctx->cwd, "/");

    return 0;
}


/*
 * Change the guest current working directory.
 *
 * NOTE:
 * This currently changes only the virtual cwd.
 * Actual directory existence checking will be added
 * at the VFS/syscall layer later.
 */
int vfs_chdir(
    struct vfs_context *ctx,
    const char *path
)
{
    if (!ctx || !path) {
        errno = EINVAL;
        return -1;
    }

    char absolute[PATH_MAX];
    char normalized[PATH_MAX];

    /*
     * Relative -> absolute guest path.
     */
    if (make_absolute_guest_path(
            ctx,
            path,
            absolute,
            sizeof(absolute)) < 0) {
        return -1;
    }

    /*
     * Normalize guest path.
     */
    if (vfs_normalize_guest_path(
            absolute,
            normalized,
            sizeof(normalized)) < 0) {
        return -1;
    }

    memcpy(
        ctx->cwd,
        normalized,
        strlen(normalized) + 1
    );

    return 0;
}


/*
 * Resolve a guest path.
 *
 * This is the main entry point of the path layer.
 *
 * Input:
 *
 *     guest path
 *
 * Output:
 *
 *     result->guest
 *     result->host
 */
int vfs_resolve(
    const struct vfs_context *ctx,
    const char *path,
    struct vfs_path *result
)
{
    if (!ctx || !path || !result) {
        errno = EINVAL;
        return -1;
    }

    memset(
        result,
        0,
        sizeof(*result)
    );

    char absolute_guest[PATH_MAX];

    /*
     * Step 1:
     *
     * Convert relative path to absolute guest path.
     */
    if (make_absolute_guest_path(
            ctx,
            path,
            absolute_guest,
            sizeof(absolute_guest)) < 0) {
        return -1;
    }

    /*
     * Step 2:
     *
     * Normalize guest path.
     */
    if (vfs_normalize_guest_path(
            absolute_guest,
            result->guest,
            sizeof(result->guest)) < 0) {
        return -1;
    }

    /*
     * Step 3:
     *
     * Map guest namespace onto rootfs.
     *
     * Guest:
     *
     *     /
     *     /etc/passwd
     *
     * Host:
     *
     *     /path/to/rootfs
     *     /path/to/rootfs/etc/passwd
     */
    const char *suffix;

    if (strcmp(result->guest, "/") == 0) {
        suffix = "";
    } else {
        suffix = result->guest;
    }

    int ret = snprintf(
        result->host,
        sizeof(result->host),
        "%s%s",
        ctx->rootfs,
        suffix
    );

    if (ret < 0) {
        errno = EINVAL;
        return -1;
    }

    if ((size_t)ret >= sizeof(result->host)) {
        errno = ENAMETOOLONG;
        return -1;
    }

    return 0;
}
