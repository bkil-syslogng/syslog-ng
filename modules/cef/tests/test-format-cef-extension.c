/*
 * Copyright (c) 2015 Balabit
 * Copyright (c) 2015 bkil.hu
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
 */

#include "template_lib.h"
#include "apphook.h"
#include "plugin.h"
#include "cfg.h"

#include <stdarg.h>

#define _OK(...) _ok_va(__VA_ARGS__, NULL)
#define _DROP_MSG(...) _ok_va("", __VA_ARGS__, NULL)
#define _DROP_PROPERTY(...) _drop_property_va(__VA_ARGS__, NULL)
#define _OK_FORMAT(X, ...) _ok_format_va(X, __VA_ARGS__, NULL);

static void
_assert(const gchar *expected, va_list ap)
{
  LogMessage *msg = message_from_list(ap);

  assert_template_format_msg ("$(format-cef-extension --prefix .cef.)", expected, msg);
  log_msg_unref (msg);
}

static void
_ok_va(const gchar *expected, ...)
{
  va_list ap;
  configuration->template_options.on_error = ON_ERROR_DROP_MESSAGE | ON_ERROR_SILENT;

  va_start (ap, expected);
  _assert(expected, ap);
  va_end (ap);
}

static void
_drop_property_va(const gchar *expected, ...)
{
  va_list ap;
  configuration->template_options.on_error = ON_ERROR_DROP_PROPERTY | ON_ERROR_SILENT;

  va_start (ap, expected);
  _assert(expected, ap);
  va_end (ap);
}

static void
_ok_format_va(const gchar *format, const gchar *expected, ...)
{
  va_list ap;
  va_start (ap, expected);
  LogMessage *msg = message_from_list(ap);
  va_end (ap);

  configuration->template_options.on_error = ON_ERROR_DROP_MESSAGE | ON_ERROR_SILENT;
  assert_template_format_msg (format, expected, msg);
  log_msg_unref (msg);
}

static void
_test_filter (void)
{
  _OK("k=v", ".cef.k", "v", "x", "w");
}

static void
_test_space_multiple (void)
{
  _OK("act=c:/program files dst=10.0.0.1",
      ".cef.act", "c:/program files",
      ".cef.dst", "10.0.0.1");
}

static void
_test_multiple (void)
{
  _OK("k=v x=y",
      ".cef.k", "v",
      ".cef.x", "y");
}

static void
_test_drop_property(void)
{
  _DROP_PROPERTY("kkk=v",
                 ".cef.a|b", "c",
                 ".cef.kkk", "v",
                 ".cef.x=y", "w");
}

static void
_test_drop_message(void)
{
  _DROP_MSG(".cef.a|b", "c",
            ".cef.kkk", "v",
            ".cef.x=y", "w");
}

static void
_test_empty(void)
{
  _OK("");
}

static void
_test_inline(void)
{
  _OK_FORMAT ("$(format-cef-extension --prefix .cef. .cef.k=v)", "k=v");
}

static void
_test_space (void)
{
  _OK ("act=blocked a ping", ".cef.act", "blocked a ping");
}

static void
_test_charset (void)
{
  _DROP_MSG (".cef.árvíztűrőtükörfúrógép", "v");
  _OK("k=árvíztűrőtükörfúrógép", ".cef.k", "árvíztűrőtükörfúrógép");
}

static void
_test_escaping (void)
{
  _OK ("act=\\\\", ".cef.act", "\\");
  _OK ("act=\\\\\\\\", ".cef.act", "\\\\");
  _OK ("act=\\=", ".cef.act", "=");
  _OK ("act=|", ".cef.act", "|");
  _OK ("act=\\n", ".cef.act", "\n");
  _OK ("act=\\r", ".cef.act", "\r");
  _OK ("act=v\\n", ".cef.act", "v\n");
  _OK ("act=v\\r", ".cef.act", "v\r");
  _OK ("act=u\\nv", ".cef.act", "u\nv");
  _OK ("act=\\r\\n", ".cef.act", "\r\n");
  _OK ("act=\\n\\r", ".cef.act", "\n\r");
  _OK ("act=this is a long value \\= something",
       ".cef.act", "this is a long value = something");

  _DROP_MSG (".cef.k=w", "v");
  _DROP_MSG (".cef.k|w", "v");
  _DROP_MSG (".cef.k\\w", "v");
  _DROP_MSG (".cef.k\nw", "v");
  _DROP_MSG (".cef.k w", "v");
}

static void
_test_prefix(void)
{
  configuration->template_options.on_error = ON_ERROR_DROP_MESSAGE | ON_ERROR_SILENT;

  _OK_FORMAT("$(format-cef-extension --prefix ..)", "k=v", "..k", "v");
  _OK_FORMAT("$(format-cef-extension --prefix ..)", "k=v", "..k", "v");
  _OK_FORMAT("$(format-cef-extension --prefix ,)", "k=v", ",k", "v");
  _OK_FORMAT("$(format-cef-extension --prefix .cef.)", "", "k", "v");
  _OK_FORMAT("$(format-cef-extension --prefix ' ')", "k=v", " k", "v");
  _OK_FORMAT("$(format-cef-extension --prefix \" \")", "k=v", " k", "v");

  _OK_FORMAT("$(format-cef-extension x=y)", "x=y", "k", "v");
  _OK_FORMAT("$(format-cef-extension)", "", "k", "v");

  assert_template_failure("$(format-cef-extension --prefix)",
                         "Missing argument for --prefix");
  assert_template_failure("$(format-cef-extension --prefix '')",
                         "Missing argument for --prefix");
  assert_template_failure("$(format-cef-extension --prefix \"\")",
                         "Missing argument for --prefix");
}

static void
_test_macro_parity(void)
{
  _OK("", "k");
  _OK_FORMAT("", "");
  _OK_FORMAT("", "", "k");
  _DROP_MSG("");
  _DROP_MSG("", "k");
  _DROP_PROPERTY("");
  _DROP_PROPERTY("", "k");
}

int
main(int argc, char *argv[])
{
  app_startup();
  putenv("TZ=UTC");
  tzset();
  init_template_tests();
  plugin_load_module("cef-plugin", configuration, NULL);

  _test_filter ();
  _test_space_multiple ();
  _test_multiple ();
  _test_drop_property();
  _test_drop_message();
  _test_empty();
  _test_inline();
  _test_space ();
  _test_charset ();
  _test_escaping ();
  _test_prefix();
  _test_macro_parity();

  deinit_template_tests();
  app_shutdown();
}
