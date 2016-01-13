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

#include "logmsg-serializer.h"
#include "plugin.h"
#include "plugin-types.h"

#define LOGMSG_SERIALIZER_ERROR log_template_error_quark()

GQuark log_template_error_quark(void);

enum LogMsgSerializerError
{
  LOGMSG_SERIALIZER_NOT_FOUND
};


LogMsgSerializer *
log_msg_serializer_factory(GlobalConfig *cfg, const gchar *name, GError **error)
{
  LogMsgSerializer *self;
  Plugin *p = plugin_find(cfg, LL_CONTEXT_SERIALIZER, name);
  if (!p)
    {
      g_set_error(error, LOGMSG_SERIALIZER_ERROR, LOGMSG_SERIALIZER_NOT_FOUND, "Unknown serializer name \"%s\"", name);
      goto error;
    }
  self = plugin_construct(p, cfg, LL_CONTEXT_SERIALIZER, name);
  return self;
error:
  return NULL;
}
