/*
 * Copyright (c) 2002-2016 BalaBit IT Ltd, Budapest, Hungary
 * Copyright (c) 2009-2016 Viktor Juhasz <viktor.juhasz@balabit.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */

#ifndef TEST_DISKQ_TOOLS_H_
#define TEST_DISKQ_TOOLS_H_

#include "syslog-ng.h"
#include "logmsg-serializer.h"
#include "diskq-options.h"
#include "testutils.h"

static inline LogMsgSerializer *
_construct_serializer()
{
  GError *error = NULL;
  LogMsgSerializer *serializer = log_msg_serializer_factory(configuration, "builtin", &error);
  assert_not_null(serializer, "Can't load builting serializer");
  return serializer;
}

static inline void
_construct_options(DiskQueueOptions *options, guint64 size, gint mem_size, gboolean reliable)
{
  memset(options, 0, sizeof(DiskQueueOptions));
  options->serializer = _construct_serializer();
  options->disk_buf_size = size;
  options->mem_buf_length = mem_size;
  options->mem_buf_size = mem_size;
  options->qout_size = 0;
  options->reliable = reliable;
}


#endif /* TEST_DISKQ_TOOLS_H_ */
