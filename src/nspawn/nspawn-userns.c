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

#include "nspawn-userns.h"
#include "user-util.h"
#include "path-util.h"

int userns_lchown(
                UserNamespaceMode userns_mode,
                uid_t uid_shift,
                uid_t uid_range,
                const char *p,
                uid_t uid,
                gid_t gid) {

        assert(p);

        if (userns_mode == USER_NAMESPACE_NO)
                return 0;

        if (uid == UID_INVALID && gid == GID_INVALID)
                return 0;

        if (uid != UID_INVALID) {
                uid += uid_shift;

                if (uid < uid_shift || uid >= uid_shift + uid_range)
                        return -EOVERFLOW;
        }

        if (gid != GID_INVALID) {
                gid += (gid_t) uid_shift;

                if (gid < (gid_t) uid_shift || gid >= (gid_t) (uid_shift + uid_range))
                        return -EOVERFLOW;
        }

        if (lchown(p, uid, gid) < 0)
                return -errno;

        return 0;
}

int userns_mkdir(
                UserNamespaceMode userns_mode,
                uid_t uid_shift,
                uid_t uid_range,
                const char *root,
                const char *path,
                mode_t mode,
                uid_t uid,
                gid_t gid) {

        const char *q;

        q = prefix_roota(root, path);
        if (mkdir(q, mode) < 0) {
                if (errno == EEXIST)
                        return 0;
                return -errno;
        }

        return userns_lchown(userns_mode, uid_shift, uid_range, q, uid, gid);
}
