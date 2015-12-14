/*
 * Copyright (c) 2002-2015 Balabit
 * Copyright (c) 2015 Viktor Juhasz <viktor.juhasz@balabit.com>
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

#ifndef LOGMSG_SERIALIZER
#define LOGMSG_SERIALIZER

#include "syslog-ng.h"
#include "serialize.h"

typedef struct _LogMsgSerializer LogMsgSerializer;

struct _LogMsgSerializer
{
  gboolean (*log_msg_serialize)(LogMsgSerializer *self, LogMessage *msg, SerializeArchive *sa);
  gboolean (*log_msg_deserialize)(LogMsgSerializer *self, LogMessage *msg, SerializeArchive *sa);
  void (*free_fn)(LogMsgSerializer *self);
};

static inline gboolean
log_msg_serialize(LogMsgSerializer *self, LogMessage *msg, SerializeArchive *sa)
{
  if (self->log_msg_serialize)
    return self->log_msg_serialize(self, msg, sa);
  return FALSE;
}

static inline void
log_msg_serializer_free(LogMsgSerializer *self)
{
  if (self->free_fn)
    {
      self->free_fn(self);
    }
}

static inline gboolean
log_msg_deserialize(LogMsgSerializer *self, LogMessage *msg, SerializeArchive *sa)
{
  if (self->log_msg_deserialize)
    return self->log_msg_deserialize(self, msg, sa);
  return FALSE;
}

LogMsgSerializer *log_msg_serializer_factory(GlobalConfig *cfg, const gchar *name, GError **error);

#endif
