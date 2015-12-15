/*
 * Copyright (c) 2002-2015 BalaBit IT Ltd, Budapest, Hungary
 * Copyright (c) 2015 Viktor Juhász
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

#include "cfg-parser.h"
#include "plugin.h"
#include "plugin-types.h"
#include "builtin-serializer.h"



static Plugin basic_serializer_plugins[] = {
  {
  .type = LL_CONTEXT_SERIALIZER,
  .name = "builtin",
  .construct = builtin_serializer_constructor,
  }
};

gboolean
builtin_serializer_module_init(GlobalConfig *cfg, CfgArgs *args)
{
  plugin_register(cfg, basic_serializer_plugins, G_N_ELEMENTS(basic_serializer_plugins));
  return TRUE;
}

const ModuleInfo module_info =
{
  .canonical_name = "builtin-serializer",
  .version = SYSLOG_NG_VERSION,
  .description = "The builtin-serializer module provide the builtin log message serializer plugin",
  .core_revision = SYSLOG_NG_SOURCE_REVISION,
  .plugins = basic_serializer_plugins,
  .plugins_len = G_N_ELEMENTS(basic_serializer_plugins),
};
