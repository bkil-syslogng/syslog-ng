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
#include "host-id.h"
#include "run-id.h"
#include "service-management.h"
#include <unistd.h> // DEBUG
#include "mainloop-call.h"
#include "mainloop-io-worker.h"

extern gboolean debug_flag; // mainloop.h
extern gboolean verbose_flag; // mainloop.h
extern gboolean trace_flag; // mainloop.h
extern gboolean log_stderr; // mainloop.h
extern GList *internal_messages; // testutils.h

static int _test_ret_num = 0;
#define TEST_FAILED (_test_ret_num = 1)

#define EXPECT_MSG(pattern, error_message) (\
    assert_grabbed_messages_contain_non_fatal(pattern, error_message, NULL) ?\
        TRUE : (TEST_FAILED,FALSE))

static void
_main_loop_deinit(void)
{
  main_loop_call_deinit();
  main_loop_io_worker_deinit();
  main_loop_worker_deinit();
}

static void
_teardown(void)
{
  service_management_publish_status("Shutting down...");

  _main_loop_deinit();

  app_shutdown();
  z_mem_trace_dump();
  g_process_startup_failed(_test_ret_num, TRUE);
}

static void
_main_loop_init_events(void)
{
}

static void
_setup_signals(void)
{
}

static void
_main_loop_init(void)
{
  service_management_publish_status("Starting up...");

  main_thread_handle = get_thread_id();
  main_loop_worker_init();
  main_loop_io_worker_init();
  main_loop_call_init();

  _main_loop_init_events();
  _setup_signals();
}


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
  log_stderr = FALSE;

  g_process_set_mode(G_PM_FOREGROUND);
  g_process_set_name(__FILE__);

  g_process_start();
  app_startup();
  _main_loop_init();

  app_post_daemonized();
}

#define URI_MSG_FMT ("Initializing MongoDB destination; " \
    "uri='%s', db='%s', collection='%s', driver='d_mongo#0'")

static gboolean
_expect_uri(const gchar *uri, const gchar *db, const gchar *col)
{
  GString *pattern = g_string_sized_new(0);
  g_string_append_printf(pattern, URI_MSG_FMT, uri, db, col);
  gboolean ok = assert_grabbed_messages_contain_non_fatal(pattern->str, "mismatch", NULL);
  g_string_free(pattern, TRUE);
  if (!ok)
    TEST_FAILED;
  return ok;
}

static gboolean
_run_test(const gchar *mongo_config)
{
  reset_grabbed_messages();

  GlobalConfig *test_cfg = cfg_new(0x0308);
  if (!test_cfg)
    {
      msg_error("Can't create new configuration", NULL);
      return FALSE;
    }

  plugin_load_candidate_modules(test_cfg);
  gchar *preprocess_into = NULL;

  GString *config_string = g_string_sized_new(0);
  g_string_append_printf(
      config_string,
      "destination d_mongo {"
      " mongodb(%s);"
      "};"

      "log {"
      " destination(d_mongo);"
      "};",
      mongo_config);

  gboolean ok = cfg_load_config(test_cfg, config_string->str,
                                syntax_only, preprocess_into);
  g_string_free(config_string, TRUE);
  msg_trace("after cfg_load_config()", NULL);

  if (!ok)
    {
      msg_error("Syntax error in configuration", NULL);
      cfg_free(test_cfg);
      return ok;
    }

  const gchar *persist_filename = "";
  test_cfg->state = persist_state_new(persist_filename);

  start_grabbing_messages();
  ok = cfg_init(test_cfg);
  stop_grabbing_messages();

  msg_trace("after cfg_init()", NULL);

  if (!ok)
    msg_error("Failed to initialize configuration", NULL);

  sleep(1);
  msg_trace("before app_post_config_loaded()", NULL);
  app_post_config_loaded();

  service_management_indicate_readiness();
  service_management_clear_status();
//  iv_main();

  msg_trace("before cfg_deinit()", NULL);
  gboolean ok2 = cfg_deinit(test_cfg);
  msg_debug("before persist_config_free()",
            evt_tag_int("persist==NULL", test_cfg->persist == NULL), NULL);
  if (test_cfg->persist)
    {
      persist_config_free(test_cfg->persist);
      test_cfg->persist = NULL;
    }
  msg_trace("before cfg_free()", NULL);
  cfg_free(test_cfg);

  if (!ok2)
    {
      msg_error("Failed to deinitialize configuration", NULL);
    }

  return ok && ok2;
}

static gboolean
_expect(const gchar *mongo_config, const gchar *expected_uri, const gchar *db, const gchar *coll)
{
  testcase_begin("%s(%s,%s,%s,%s)", __FUNCTION__, mongo_config, expected_uri, db, coll);

  gboolean ok = _run_test(mongo_config);
  ok &= _expect_uri(expected_uri, db, coll);
  if (!ok)
    TEST_FAILED;

  testcase_end();
  return ok;
}

static gboolean
_error(const gchar *mongo_config, const gchar *error)
{
  testcase_begin("%s(%s,%s)", __FUNCTION__, mongo_config, error);

  gboolean ok = !_run_test(mongo_config);
  ok &= EXPECT_MSG(error, "mismatch");
  if (!ok)
    TEST_FAILED;

  testcase_end();
  return ok;
}

int
main(int argc, char **argv)
{
  _setup(argc, argv);

  _expect("uri('mongodb://127.0.0.1:27017/syslog')",
          "mongodb://127.0.0.1:27017/syslog",
          "syslog", "messages");

  _error("uri('mongodb://127.0.0.1:27017/syslog')",
         "Initializing MongoDB destination; uri='mongodb://127.0.0.1:27017/syslog', db='syslog', collection='messages', driver='d_mongo#0'");

  _expect("",
          "mongodb://127.0.0.1:27017/syslog?slaveOk=true&sockettimeoutms=60000",
          "syslog", "messages");
/*
  _error("uri('syslog-ng')",
         "Error parsing MongoDB URI; uri='syslog-ng', driver='d_mongo#0'");
*/
  _teardown();
  return 1;
}
