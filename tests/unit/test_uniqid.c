/*
 * Copyright (c) 2015 Balabit
 * Copyright (c) 2015 Laszlo Budai
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

#include "testutils.h"
#include "syslog-ng.h"
#include "logmsg.h"
#include "template/templates.h"
#include "apphook.h"
#include "template/macros.h"
#include <stdio.h>
#include <string.h>

#define UNIQID_TESTCASE(testfunc, ...)  { testcase_begin("%s(%s)", #testfunc, #__VA_ARGS__); testfunc(__VA_ARGS__); testcase_end(); }

static void
test_uniqid_template_format()
{
  LogMessage *msg = log_msg_new_mark();
  const gchar *expected="cafebabe@0000000000000afc";

  msg->host_id = 0xcafebabe;
  msg->rcptid = 0xafc;
  gssize len = 0;

  const gchar *value = log_msg_get_macro_value(msg, log_macro_lookup("UNIQID", strlen("UNIQID")+1), &len);

  assert_true(strcmp(value, expected) == 0,
              "expanding M_UNIQID macro failed; actual value:[%s], expected:[%s]",
              value,
              expected);
}

int main(int argc, char **argv)
{
  app_startup();

  UNIQID_TESTCASE(test_uniqid_template_format);

  app_shutdown();

  return 0;
}
