/*
 * Copyright (c) 2008-2014 Balabit
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

#include "logmsg/logmsg.h"
#include "apphook.h"
#include "cfg.h"
#include "plugin.h"
#include "msg_parse_lib.h"
#include "testutils.h"

#include <stdlib.h>
#include <string.h>

static void
testcase_update_sdata(const gchar *msg, const gchar *expected_sd_str, const gchar *elem_name1, ...)
{
  LogMessage *logmsg;
  GString *sd_str = g_string_new("");
  va_list va;
  const gchar *elem, *param, *value;

  parse_options.flags |= LP_SYSLOG_PROTOCOL;

  va_start(va, elem_name1);

  logmsg = log_msg_new(msg, strlen(msg), NULL, &parse_options);

  elem = elem_name1;
  param = va_arg(va, char *);
  value = va_arg(va, char *);
  while (elem)
    {
      gchar sd_name[64];

      g_snprintf(sd_name, sizeof(sd_name), ".SDATA.%s.%s", elem, param);
      log_msg_set_value_by_name(logmsg, sd_name, value, -1);
      elem = va_arg(va, char *);
      param = va_arg(va, char *);
      value = va_arg(va, char *);
    }

  log_msg_format_sdata(logmsg, sd_str, 0);

  assert_string(sd_str->str, expected_sd_str, "sdata update failed");

  g_string_free(sd_str, TRUE);
  log_msg_unref(logmsg);
}

int
main(int argc G_GNUC_UNUSED, char *argv[] G_GNUC_UNUSED)
{
  app_startup();
  init_and_load_syslogformat_module();

  testcase_update_sdata("<132>1 2006-10-29T01:59:59.156+01:00 mymachine evntslog - - [exampleSDID@0 iut=\"3\" eventSource=\"Application\" eventID=\"1011\"][examplePriority@0 class=\"high\"] An application event log entry...",
                  "[exampleSDID@0 iut=\"3\" eventSource=\"Application\" eventID=\"1011\"][examplePriority@0 class=\"high\"][meta sequenceId=\"11\"][syslog-ng param=\"value\"]",
                  "meta", "sequenceId", "11",
                  "syslog-ng", "param", "value",
                  NULL, NULL, NULL);

  deinit_syslogformat_module();
  app_shutdown();
  return 0;
}
