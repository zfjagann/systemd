/* SPDX-License-Identifier: LGPL-2.1+ */
#pragma once

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

#include <sys/types.h>

#include "nspawn-settings.h"

int userns_lchown(UserNamespaceMode userns_mode, uid_t uid_shift, uid_t uid_range, const char *p, uid_t uid, gid_t gid);
int userns_mkdir(UserNamespaceMode userns_mode, uid_t uid_shift, uid_t uid_range, const char *root, const char *path, mode_t mode, uid_t uid, gid_t gid);
