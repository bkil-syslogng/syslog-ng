/*
 * Copyright (c) 2013 Balabit
 * Copyright (c) 2013 Gergely Nagy <algernon@balabit.hu>
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

#include "mainloop.h"
#include "testutils.h"
#include "cfg-lexer.h"
#include "cfg-grammar.h"
#include "messages.h"
#include "memtrace.h"
#include "apphook.h"
#include "gprocess.h"

extern gboolean debug_flag; // mainloop.h
extern gboolean verbose_flag; // mainloop.h
extern gboolean trace_flag; // mainloop.h
extern gboolean log_stderr; // mainloop.h
extern GList *internal_messages; // testutils.h

int _test_ret_num = 0;

static void
_setup(int argc, char **argv)
{
  z_mem_trace_init("syslog-ng.trace");
  g_process_set_argv_space(argc, (gchar **) argv);
  main_loop_global_init();

  syntax_only = FALSE;
  debug_flag = TRUE;
  verbose_flag = TRUE;
  trace_flag = TRUE;
  log_stderr = TRUE;

  g_process_set_mode(G_PM_FOREGROUND);
  g_process_set_name(__FILE__);

  g_process_start();
  app_startup();
  main_loop_init();
}

static void
_teardown(void)
{
  g_process_startup_failed(_test_ret_num, TRUE);

  main_loop_deinit();

  app_shutdown();
  z_mem_trace_dump();
  g_process_finish();
}

static int
_run_test(const char *input, const char *output)
{
  GString *config_string = g_string_sized_new(0);
  g_string_append_printf(
      config_string,
      "source s_internal { internal(); };"

      "destination d_mongo {"
      " mongodb(%s);"
      "};"

      "log {"
      " source(s_internal);"
      " destination(d_mongo);"
      "};",
      input);

  GlobalConfig *current_configuration = cfg_new(0x0308);
  plugin_load_candidate_modules(current_configuration);
  gchar *preprocess_into = NULL;
  start_grabbing_messages();
  gboolean ok = cfg_load_config(current_configuration, config_string->str,
                                syntax_only, preprocess_into);
  g_string_free(config_string, TRUE);
  msg_trace("after cfg_load_config()", NULL);
  stop_grabbing_messages();

  if (!ok)
    {
      msg_error("Syntax error in configuration", NULL);
      cfg_free(current_configuration);
      return 1;
    }

  cfg_free(current_configuration);

  msg_debug("TODO: check whether output matches", evt_tag_str("output", output), NULL);

  GList *l;

    for (l = internal_messages; l; l = l->next)
      {
        LogMessage *msg = (LogMessage *) l->data;
        const gchar *msg_text = log_msg_get_value(msg, LM_V_MESSAGE, NULL);

        msg_debug("recorded message", evt_tag_str("msg_text", msg_text), NULL);
        printf("recorded %s\n", msg_text);
      }

  return 0;
}

static void
_expect(const char *input, const char *output)
{
  testcase_begin("_expect(%s,%s)", input, output);

  _test_ret_num = _run_test(input, output);

  testcase_end();
}

int
main(int argc, char **argv)
{
  _setup(argc, argv);

  _expect("uri('szia')", "szia");
  _expect("uri('szia2')", "szia2");
  _expect("", "mongodb://localhost:27012");

  _teardown();
  return 1;
}
