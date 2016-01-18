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
#include "testutils.h"
#include "syslog-ng.h"
#include "apphook.h"
#include "logmsg-serializer.h"
#include "cfg.h"
#include "plugin.h"
#include "plugin-types.h"

GlobalConfig *cfg;

#define MESSAGE "this is a test message"

typedef struct _LogMsgSerializerMock
{
  LogMsgSerializer super;
} LogMsgSerializerMock;


static gboolean
_serialize(LogMsgSerializer *self, LogMessage *msg, SerializeArchive *sa)
{
  GString *string = g_string_new(log_msg_get_value(msg, LM_V_MESSAGE, NULL));
  gboolean result = serialize_write_string(sa, string);
  g_string_free(string, TRUE);
  return result;
}

static gboolean
_deserialize(LogMsgSerializer *self, LogMessage *msg, SerializeArchive *sa)
{
  GString *string = g_string_sized_new(1024);
  gboolean result =  serialize_read_string(sa, string);
  result &= (strcmp(MESSAGE, string->str) == 0);
  g_string_free(string, TRUE);
  return result;
}

static LogMsgSerializer *
log_msg_serializer_mock_new()
{
  LogMsgSerializerMock *self = g_new0(LogMsgSerializerMock, 1);
  self->super.log_msg_serialize = _serialize;
  self->super.log_msg_deserialize = _deserialize;
  self->super.free_fn = (void *)(LogMsgSerializerMock *)g_free;
  return &self->super;
}

static gpointer
_constructor(Plugin *self, GlobalConfig *cfg, gint plugin_type, const gchar *plugin_name)
{
  return log_msg_serializer_mock_new();
}

static Plugin MockSerializer =
{
  .type = LL_CONTEXT_SERIALIZER,
  .name = "mock",
  .construct = _constructor,
};

void
test()
{
  GError *error = NULL;
  LogMsgSerializer *serializer = log_msg_serializer_factory(cfg, "mock", &error);
  assert_not_null(serializer, "Can't find plugin error: %s", error ? error->message : "");
  LogMessage *msg = log_msg_new_empty();
  log_msg_set_value(msg, LM_V_MESSAGE, MESSAGE, strlen(MESSAGE));
  GString *buffer = g_string_sized_new(1024);
  SerializeArchive *sa = serialize_string_archive_new(buffer);
  assert_true(log_msg_serialize(serializer, msg, sa), "Failed to serialize");
  assert_true(log_msg_deserialize(serializer, msg, sa), "Failed to read back");
  log_msg_serializer_free(serializer);
  log_msg_unref(msg);
  serialize_archive_free(sa);
  g_string_free(buffer, TRUE);
}

int
main(int argc G_GNUC_UNUSED, char *argv[] G_GNUC_UNUSED)
{
  
  app_startup();
  cfg = cfg_new(0x0307);
  plugin_register(cfg, &MockSerializer, 1);
  test();
  return 0;
}
