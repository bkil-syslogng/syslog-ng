/*
 * Copyright (c) 2016 Balabit
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
#include "lib/plugin.h"
#include "resolved-configurable-paths.h"

extern GList *internal_messages; // testutils.h

static guint _test_ret_num = 0;
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
_g_process_finish(void)
{
  g_process_restore_environ();
}

static void
_teardown(void)
{
  service_management_publish_status("Shutting down...");

  _main_loop_deinit();

  app_shutdown();
  z_mem_trace_dump();
  _g_process_finish();
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
  resolved_configurable_paths_init(&resolvedConfigurablePaths);

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

static gboolean
_run_test(const gchar *mongo_config)
{
  reset_grabbed_messages();

  GlobalConfig *test_cfg = cfg_new(0x0308);
  if (!test_cfg)
    {
      msg_error("Can't create new configuration", NULL, NULL);
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

  start_grabbing_messages();
  gboolean ok = cfg_load_config(test_cfg, config_string->str,
                                syntax_only, preprocess_into);
  g_string_free(config_string, TRUE);
  msg_trace("after cfg_load_config()", NULL, NULL);

  if (!ok)
    {
      msg_error("Syntax error in configuration", evt_tag_str("mongo_config", mongo_config), NULL);
      cfg_free(test_cfg);
      return FALSE;
    }

  const gchar *persist_filename = "";
  test_cfg->state = persist_state_new(persist_filename);

  ok = cfg_init(test_cfg);
  stop_grabbing_messages();

  msg_trace("after cfg_init()", NULL, NULL);

  if (!ok)
    msg_error("Failed to initialize configuration", evt_tag_str("mongo_config", mongo_config), NULL);

  sleep(1);
  msg_trace("before app_post_config_loaded()", NULL, NULL);
  app_post_config_loaded();

  service_management_indicate_readiness();
  service_management_clear_status();
//  iv_main();

  msg_trace("before cfg_deinit()", NULL, NULL);
  gboolean ok2 = cfg_deinit(test_cfg);
  msg_debug("before persist_config_free()",
            evt_tag_int("persist==NULL", test_cfg->persist == NULL), NULL);
  if (test_cfg->persist)
    {
      persist_config_free(test_cfg->persist);
      test_cfg->persist = NULL;
    }
  msg_trace("before cfg_free()", NULL, NULL);
  cfg_free(test_cfg);
  msg_trace("after cfg_free()", NULL, NULL);

  if (!ok2)
    {
      msg_error("Failed to deinitialize configuration", evt_tag_str("mongo_config", mongo_config), NULL);
    }

  return ok && ok2;
}

#define URI_MSG_FMT ("Initializing MongoDB destination; " \
    "uri='mongodb://%s', db='%s', collection='%s', driver='d_mongo#0'")

static gboolean
_expect_uri_in_log(const gchar *uri, const gchar *db, const gchar *col)
{
  GString *pattern = g_string_sized_new(0);
  g_string_append_printf(pattern, URI_MSG_FMT, uri, db, col);
  gboolean ok = assert_grabbed_messages_contain_non_fatal(pattern->str, "mismatch", NULL);
  g_string_free(pattern, TRUE);
  reset_grabbed_messages();
  if (!ok)
    TEST_FAILED;
  return ok;
}

static gboolean
_run_and_expect_uri(const gchar *mongo_config, const gchar *expected_uri, const gchar *db, const gchar *coll)
{
  testcase_begin("%s(%s,%s,%s,%s)", __FUNCTION__, mongo_config, expected_uri, db, coll);

  gboolean ok = _run_test(mongo_config);
  ok &= _expect_uri_in_log(expected_uri, db, coll);
  if (!ok)
    TEST_FAILED;

  testcase_end();
  return ok;
}

static gboolean
_run_and_expect_error(const gchar *mongo_config, const gchar *error)
{
  testcase_begin("%s(%s,%s)", __FUNCTION__, mongo_config, error);

  gboolean ok = !_run_test(mongo_config);
  if (!ok)
    msg_error("_run_test() accepted, but it should have failed", NULL, NULL);
  ok &= EXPECT_MSG(error, "mismatch");
  reset_grabbed_messages();
  if (!ok)
    TEST_FAILED;

  testcase_end();
  return ok;
}

#define DEFAULTOPTS "?slaveOk=true&sockettimeoutms=60000"

typedef struct _URITestCase
{
  const char *mongo_config;
  const char *expected_uri;
  const char *expected_db;
  const char *expected_collection;
} URITestCase;

typedef struct _ErrorTestCase
{
  const char *mongo_config;
  const char *expected_error_message;
} ErrorTestCase;

static gboolean
_correct_uri_cases(const URITestCase uri_tests[])
{
  gboolean ok = TRUE;
  const URITestCase *test = uri_tests;
  while (test->mongo_config)
    {
      ok &= _run_and_expect_uri(test->mongo_config, test->expected_uri, test->expected_db, test->expected_collection);
      test++;
    }
  return ok;
}

static gboolean
_error_case(const ErrorTestCase uri_test)
{
  return _run_and_expect_error(uri_test.mongo_config, uri_test.expected_error_message);
}

static gboolean
_error_cases(const ErrorTestCase uri_tests[])
{
  gboolean ok = TRUE;
  const ErrorTestCase *test = uri_tests;
  while (test->mongo_config)
    {
      ok &= _error_case(*test++);
    }
  return ok;
}

#define INVALID_KEYWORD(word) ( \
    "Error parsing afmongodb, inner-dest plugin " \
    word \
    " not found in <string> at line 1, column 31:")

static void
_test_legacy(void)
{
#if SYSLOG_NG_ENABLE_LEGACY_MONGODB_OPTIONS
  _error_case((const ErrorTestCase)
          {
              "safe_mode(yes) uri('mongodb://127.0.0.1:27017/syslog')",
              "Error: either specify a MongoDB URI (and optional collection) or only legacy options; "
                  "driver='d_mongo#0'"
          });

  _correct_uri_cases((const URITestCase[])
          {
                {
                    "servers('127.0.0.2:27018', 'localhost:1234')",
                    "localhost:1234,127.0.0.2:27018/syslog" DEFAULTOPTS,
                    "syslog",
                    "messages"
                },
                {
                    "servers('127.0.0.2')",
                    "127.0.0.2:27017/syslog" DEFAULTOPTS,
                    "syslog",
                    "messages"
                },
                {
                    "host('localhost')",
                    "localhost:27017/syslog" DEFAULTOPTS,
                    "syslog",
                    "messages"
                },
                {
                    "host('localhost') port(1234)",
                    "localhost:1234/syslog" DEFAULTOPTS,
                    "syslog",
                    "messages"
                },
                {
                    "port(27017)",
                    "127.0.0.1:27017/syslog" DEFAULTOPTS,
                    "syslog",
                    "messages"
                },
                {
                    "port(1234)",
                    "127.0.0.1:1234/syslog" DEFAULTOPTS,
                    "syslog",
                    "messages"
                },
                {
                    "path('/tmp/mongo.sock')",
                    "/tmp/mongo.sock" DEFAULTOPTS,
                    "tmp/mongo.sock",
                    "messages" },
                {
                    "database('syslog-ng')",
                    "127.0.0.1:27017/syslog-ng" DEFAULTOPTS,
                    "syslog-ng",
                    "messages"
                },
                {
                    "safe_mode(no)",
                    "127.0.0.1:27017/syslog",
                    "syslog",
                    "messages"
                },
                {
                    "username('user') password('password')",
                    "user:password@127.0.0.1:27017/syslog" DEFAULTOPTS,
                    "syslog",
                    "messages"
                },
                {
                    "safe_mode(yes) collection('messages2')",
                    "127.0.0.1:27017/syslog" DEFAULTOPTS,
                    "syslog",
                    "messages2"
                },
                { }
          });
#else
  _error_cases((const ErrorTestCase[])
          {
                {
                    "database('syslog')",
                    INVALID_KEYWORD("database")
                },
                {
                    "username('user')",
                    INVALID_KEYWORD("username")
                },
                {
                    "password('password')",
                    INVALID_KEYWORD("password")
                },
                {
                    "safe_mode(yes)",
                    INVALID_KEYWORD("safe_mode")
                },
                {
                    "host('localhost')",
                    INVALID_KEYWORD("host")
                },
                {
                    "port(1234)",
                    INVALID_KEYWORD("port")
                },
                {
                    "path('/tmp/syslog.sock')",
                    INVALID_KEYWORD("path")
                },
                {
                    "servers('localhost:1234')",
                    INVALID_KEYWORD("servers")
                },
                { }
          });
#endif
}

static void
_test_uri(void)
{
  _error_cases((const ErrorTestCase[])
          {
                {
                    "uri()",
                    "Error parsing afmongodb, syntax error, unexpected \\')\\', expecting LL_IDENTIFIER or "
                        "LL_STRING in <string> at line 1, column 35:"
                },
                {
                    "INVALID-KEYWORD()",
                    INVALID_KEYWORD("INVALID-KEYWORD")
                },
                {
                    "uri('INVALID-URI')",
                    "Error parsing MongoDB URI; uri='INVALID-URI', driver='d_mongo#0'"
                },
                {
                    "uri('mongodb://127.0.0.1:27017/')",
                    "Missing DB name from MongoDB URI; uri='mongodb://127.0.0.1:27017/', driver='d_mongo#0'"
                },
                { }
          });

  _correct_uri_cases((const URITestCase[])
          {
                {
                    "",
                    "127.0.0.1:27017/syslog" DEFAULTOPTS,
                    "syslog",
                    "messages"
                },
                {
                    "uri('mongodb:///tmp/mongo.sock')",
                    "/tmp/mongo.sock",
                    "tmp/mongo.sock",
                    "messages"
                },
                {
                    "uri('mongodb://127.0.0.1:27017/syslog')",
                    "127.0.0.1:27017/syslog",
                    "syslog",
                    "messages"
                },
                {
                    "collection('messages2')",
                    "127.0.0.1:27017/syslog" DEFAULTOPTS,
                    "syslog",
                    "messages2"
                },
                { }
          });
}

int
main(int argc, char **argv)
{
  _setup(argc, argv);

  _test_legacy();
  _test_uri();

  _teardown();
  return (int) _test_ret_num;
}
