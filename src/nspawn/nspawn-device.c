/* SPDX-License-Identifier: LGPL-2.1+ */
/***
  This file is part of systemd.

  Copyright 2015 Lennart Poettering

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include <sys/mount.h>

#include "alloc-util.h"
#include "fs-util.h"
#include "mount-util.h"
#include "nspawn-device.h"
#include "nspawn-userns.h"
#include "path-util.h"
#include "umask-util.h"
#include "util.h"

int copy_devnodes(const char *dest, UserNamespaceMode userns_mode, uid_t uid_shift, uid_t uid_range) {

        static const char devnodes[] =
                "null\0"
                "zero\0"
                "full\0"
                "random\0"
                "urandom\0"
                "tty\0"
                "net/tun\0";

        const char *d;
        int r = 0;
        _cleanup_umask_ mode_t u;

        assert(dest);

        u = umask(0000);

        /* Create /dev/net, so that we can create /dev/net/tun in it */
        if (userns_mkdir(userns_mode, uid_shift, uid_range, dest, "/dev/net", 0755, 0, 0) < 0)
                return log_error_errno(r, "Failed to create /dev/net directory: %m");

        NULSTR_FOREACH(d, devnodes) {
                _cleanup_free_ char *from = NULL, *to = NULL;
                struct stat st;

                from = strappend("/dev/", d);
                to = prefix_root(dest, from);

                if (stat(from, &st) < 0) {

                        if (errno != ENOENT)
                                return log_error_errno(errno, "Failed to stat %s: %m", from);

                } else if (!S_ISCHR(st.st_mode) && !S_ISBLK(st.st_mode)) {

                        log_error("%s is not a char or block device, cannot copy.", from);
                        return -EIO;

                } else {
                        if (mknod(to, st.st_mode, st.st_rdev) < 0) {
                                /* Explicitly warn the user when /dev is already populated. */
                                if (errno == EEXIST)
                                        log_notice("%s/dev is pre-mounted and pre-populated. If a pre-mounted /dev is provided it needs to be an unpopulated file system.", dest);
                                if (errno != EPERM)
                                        return log_error_errno(errno, "mknod(%s) failed: %m", to);

                                /* Some systems abusively restrict mknod but
                                 * allow bind mounts. */
                                r = touch(to);
                                if (r < 0)
                                        return log_error_errno(r, "touch (%s) failed: %m", to);
                                r = mount_verbose(LOG_DEBUG, from, to, NULL, MS_BIND, NULL);
                                if (r < 0)
                                        return log_error_errno(r, "Both mknod and bind mount (%s) failed: %m", to);
                        }

                        r = userns_lchown(userns_mode, uid_shift, uid_range, to, 0, 0);
                        if (r < 0)
                                return log_error_errno(r, "chown() of device node %s failed: %m", to);
                }
        }

        return r;
}

int setup_pts(const char *dest, const char *selinux_apifs_context, UserNamespaceMode userns_mode, uid_t uid_shift, uid_t uid_range) {
        _cleanup_free_ char *options = NULL;
        const char *p;
        int r;

#if HAVE_SELINUX
        if (selinux_apifs_context)
                (void) asprintf(&options,
                                "newinstance,ptmxmode=0666,mode=620,gid=" GID_FMT ",context=\"%s\"",
                                uid_shift + TTY_GID,
                                selinux_apifs_context);
        else
#endif
                (void) asprintf(&options,
                                "newinstance,ptmxmode=0666,mode=620,gid=" GID_FMT,
                                uid_shift + TTY_GID);

        if (!options)
                return log_oom();

        /* Mount /dev/pts itself */
        p = prefix_roota(dest, "/dev/pts");
        if (mkdir(p, 0755) < 0)
                return log_error_errno(errno, "Failed to create /dev/pts: %m");
        r = mount_verbose(LOG_ERR, "devpts", p, "devpts", MS_NOSUID|MS_NOEXEC, options);
        if (r < 0)
                return r;
        r = userns_lchown(userns_mode, uid_shift, uid_range, p, 0, 0);
        if (r < 0)
                return log_error_errno(r, "Failed to chown /dev/pts: %m");

        /* Create /dev/ptmx symlink */
        p = prefix_roota(dest, "/dev/ptmx");
        if (symlink("pts/ptmx", p) < 0)
                return log_error_errno(errno, "Failed to create /dev/ptmx symlink: %m");
        r = userns_lchown(userns_mode, uid_shift, uid_range, p, 0, 0);
        if (r < 0)
                return log_error_errno(r, "Failed to chown /dev/ptmx: %m");

        /* And fix /dev/pts/ptmx ownership */
        p = prefix_roota(dest, "/dev/pts/ptmx");
        r = userns_lchown(userns_mode, uid_shift, uid_range, p, 0, 0);
        if (r < 0)
                return log_error_errno(r, "Failed to chown /dev/pts/ptmx: %m");

        return 0;
}

int setup_dev_console(const char *dest, const char *console, UserNamespaceMode userns_mode, uid_t uid_shift, uid_t uid_range) {
        _cleanup_umask_ mode_t u;
        const char *to;
        int r;

        assert(dest);
        assert(console);

        u = umask(0000);

        r = chmod_and_chown(console, 0600, uid_shift, uid_shift);
        if (r < 0)
                return log_error_errno(r, "Failed to correct access mode for TTY: %m");

        /* We need to bind mount the right tty to /dev/console since
         * ptys can only exist on pts file systems. To have something
         * to bind mount things on we create a empty regular file. */

        to = prefix_roota(dest, "/dev/console");
        r = touch(to);
        if (r < 0)
                return log_error_errno(r, "touch() for /dev/console failed: %m");

        return mount_verbose(LOG_ERR, console, to, NULL, MS_BIND, NULL);
}
