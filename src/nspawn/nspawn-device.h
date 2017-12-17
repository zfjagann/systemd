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

#include "nspawn-settings.h"

int copy_devnodes(const char *dest, UserNamespaceMode userns_mode, uid_t uid_shift, uid_t uid_range);
int setup_pts(const char *dest, const char *selinux_apifs_context, UserNamespaceMode userns_mode, uid_t uid_shift, uid_t uid_range);
int setup_dev_console(const char *dest, const char *console, UserNamespaceMode userns_mode, uid_t uid_shift, uid_t uid_range);
