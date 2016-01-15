/*
 * Copyright (c) 2014 Balabit
 * Copyright (c) 2014 Gergely Nagy
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */

#include "syslog-ng.h"
#include "unix-credentials.h"


void
socket_set_pass_credentials(gint fd)
{
#if defined(CRED_PASS_SUPPORTED)
#if defined(LOCAL_CREDS) || defined(SO_PASSCRED)
  static gint one = 1;
#endif

#ifdef LOCAL_CREDS
  setsockopt(fd, 0, LOCAL_CREDS, &one, sizeof(one));
#else
#ifdef SO_PASSCRED
  setsockopt(fd, SOL_SOCKET, SO_PASSCRED, &one, sizeof(one));
#endif
#endif
#endif
}
