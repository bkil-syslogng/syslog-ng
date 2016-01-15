/*
 * Copyright (c) 2002-2016 BalaBit IT Ltd, Budapest, Hungary
 * Copyright (c) 2016 Viktor Juhasz <viktor.juhasz@balabit.com>
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

#ifndef DISKQ_OPTIONS_H_
#define DISKQ_OPTIONS_H_

#include "syslog-ng.h"
#include "logmsg-serializer.h"

#define MIN_DISK_BUF_SIZE 1024*1024

typedef struct _DiskQueueOptions
{
  gint64 disk_buf_size;
  gint qout_size;
  gboolean read_only;
  gboolean reliable;
  gint mem_buf_size;
  gint mem_buf_length;
  LogMsgSerializer *serializer;
  gchar *dir;
} DiskQueueOptions;

void disk_queue_options_qout_size_set(DiskQueueOptions *self, gint qout_size);
void disk_queue_options_disk_buf_size_set(DiskQueueOptions *self, gint64 disk_buf_size);
void disk_queue_options_reliable_set(DiskQueueOptions *self, gboolean reliable);
void disk_queue_options_mem_buf_size_set(DiskQueueOptions *self, gint mem_buf_size);
void disk_queue_options_mem_buf_length_set(DiskQueueOptions *self, gint mem_buf_length);
void disk_queue_options_check_plugin_settings(DiskQueueOptions *self);
void disk_queue_options_set_serializer(DiskQueueOptions *self, LogMsgSerializer *serializer);
void disk_queue_options_set_dir(DiskQueueOptions *self, const gchar *dir);
void disk_queue_options_set_default_options(DiskQueueOptions *self);
void disk_queue_options_destroy(DiskQueueOptions *self);

#endif /* DISKQ_OPTIONS_H_ */
