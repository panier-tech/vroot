#include "path.h"

#include <stdio.h>
#include <string.h>


/*
 * Check one path resolution.
 *
 * expected_guest:
 *     Expected canonical guest path.
 *
 * expected_suffix:
 *     Expected suffix after ctx->rootfs.
 *
 * For example:
 *
 *     rootfs = /tmp/rootfs
 *
 *     expected_suffix = /etc/passwd
 *
 *     expected host =
 *         /tmp/rootfs/etc/passwd
 */
static int check_path(
    const struct vfs_context *ctx,
    const char *input,
    const char *expected_guest,
    const char *expected_suffix
)
{
    struct vfs_path result;

    if (vfs_resolve(
            ctx,
            input,
            &result) < 0) {

        perror("vfs_resolve");
        return 1;
    }

    printf(
        "%-30s -> guest=%-25s host=%s\n",
        input,
        result.guest,
        result.host
    );

    int failed = 0;


    /*
     * Check guest path.
     */
    if (strcmp(
            result.guest,
            expected_guest) != 0) {

        fprintf(
            stderr,
            "  guest FAIL\n"
            "    expected: %s\n"
            "    actual:   %s\n",
            expected_guest,
            result.guest
        );

        failed = 1;
    }


    /*
     * Check host path.
     *
     * Instead of constructing:
     *
     *     rootfs + expected_suffix
     *
     * in another PATH_MAX buffer, compare the
     * individual components directly.
     */
    size_t root_len = strlen(ctx->rootfs);
    size_t suffix_len = strlen(expected_suffix);
    size_t host_len = strlen(result.host);


    /*
     * Check total length.
     */
    if (host_len != root_len + suffix_len) {

        fprintf(
            stderr,
            "  host FAIL\n"
            "    expected: %s%s\n"
            "    actual:   %s\n",
            ctx->rootfs,
            expected_suffix,
            result.host
        );

        failed = 1;
    }

    /*
     * Check rootfs prefix.
     */
    else if (strncmp(
                 result.host,
                 ctx->rootfs,
                 root_len) != 0) {

        fprintf(
            stderr,
            "  host FAIL\n"
            "    expected: %s%s\n"
            "    actual:   %s\n",
            ctx->rootfs,
            expected_suffix,
            result.host
        );

        failed = 1;
    }

    /*
     * Check guest-path suffix.
     */
    else if (strcmp(
                 result.host + root_len,
                 expected_suffix) != 0) {

        fprintf(
            stderr,
            "  host FAIL\n"
            "    expected: %s%s\n"
            "    actual:   %s\n",
            ctx->rootfs,
            expected_suffix,
            result.host
        );

        failed = 1;
    }


    return failed;
}


int main(void)
{
    struct vfs_context ctx;


    /*
     * Initialize VFS.
     */
    if (vfs_init(
            &ctx,
            "./rootfs") < 0) {

        perror("vfs_init");
        return 1;
    }


    printf(
        "rootfs = %s\n"
        "cwd    = %s\n\n",
        ctx.rootfs,
        ctx.cwd
    );


    int failures = 0;


    /*
     * ---------------------------------------------------------
     * Absolute paths
     * ---------------------------------------------------------
     */

    failures += check_path(
        &ctx,
        "/etc/hostname",
        "/etc/hostname",
        "/etc/hostname"
    );


    failures += check_path(
        &ctx,
        "/bin/sh",
        "/bin/sh",
        "/bin/sh"
    );


    /*
     * ---------------------------------------------------------
     * "." normalization
     * ---------------------------------------------------------
     */

    failures += check_path(
        &ctx,
        "/etc/./hostname",
        "/etc/hostname",
        "/etc/hostname"
    );


    /*
     * ---------------------------------------------------------
     * ".." normalization
     * ---------------------------------------------------------
     */

    failures += check_path(
        &ctx,
        "/etc/../bin/sh",
        "/bin/sh",
        "/bin/sh"
    );


    /*
     * ---------------------------------------------------------
     * Prevent escape above guest root
     * ---------------------------------------------------------
     */

    failures += check_path(
        &ctx,
        "/../../../../etc/passwd",
        "/etc/passwd",
        "/etc/passwd"
    );


    /*
     * ---------------------------------------------------------
     * Relative path from guest "/"
     * ---------------------------------------------------------
     */

    failures += check_path(
        &ctx,
        "etc/hostname",
        "/etc/hostname",
        "/etc/hostname"
    );


    /*
     * ---------------------------------------------------------
     * Change guest cwd
     * ---------------------------------------------------------
     */

    if (vfs_chdir(
            &ctx,
            "/etc") < 0) {

        perror("vfs_chdir");
        return 1;
    }


    printf(
        "\n"
        "after vfs_chdir(\"/etc\"):\n"
        "cwd = %s\n\n",
        ctx.cwd
    );


    /*
     * Relative path from /etc.
     */
    failures += check_path(
        &ctx,
        "hostname",
        "/etc/hostname",
        "/etc/hostname"
    );


    /*
     * Relative "..".
     */
    failures += check_path(
        &ctx,
        "../bin/sh",
        "/bin/sh",
        "/bin/sh"
    );


    /*
     * More complicated relative path.
     */
    failures += check_path(
        &ctx,
        "../usr/./bin/../bin/env",
        "/usr/bin/env",
        "/usr/bin/env"
    );


    /*
     * ---------------------------------------------------------
     * Guest root
     * ---------------------------------------------------------
     */

    failures += check_path(
        &ctx,
        "/",
        "/",
        ""
    );


    /*
     * ---------------------------------------------------------
     * Result
     * ---------------------------------------------------------
     */

    if (failures != 0) {

        fprintf(
            stderr,
            "\n%d test(s) failed\n",
            failures
        );

        return 1;
    }


    printf(
        "\n"
        "all tests passed\n"
    );

    return 0;
}
