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

#include "testutils.h"
#include "mainloop.h"
#include "modules/afmongodb/afmongodb-parser.h"

extern GList *internal_messages; // testutils.h

static guint _test_ret_num = 0;
#define TEST_FAILED (_test_ret_num = 1)

#define DEFAULTOPTS "?slaveOk=true&sockettimeoutms=60000"

static GlobalConfig *test_cfg;
static LogDriver *mongodb;

static void
_before_test(void)
{
  start_grabbing_messages();
  mongodb = afmongodb_dd_new(test_cfg);
}

static void
_after_test(void)
{
  afmongodb_dd_private_uri_init(mongodb);
  stop_grabbing_messages();
}

static void
_free_test(void)
{
  reset_grabbed_messages();
  if (mongodb)
    {
      log_pipe_unref(&mongodb->super);
      mongodb = NULL;
    }
}

static gboolean
_expect_text_in_log_helper(const gchar *testcase, const gchar *pattern)
{
  gboolean ok = assert_grabbed_messages_contain_non_fatal(pattern, "mismatch", NULL);
  reset_grabbed_messages();
  if (!ok)
    TEST_FAILED;
  return ok;
}

static void
_expect_text_in_log(const gchar *testcase, const gchar *pattern)
{
  _after_test();

  testcase_begin("%s(%s, %s)", __FUNCTION__, testcase, pattern);
  _expect_text_in_log_helper(testcase, pattern);
  testcase_end();

  _free_test();
  _before_test();
}

static void
_expect_uri_in_log(const gchar *testcase, const gchar *uri, const gchar *db, const gchar *coll)
{
  GString *pattern = g_string_sized_new(0);
  g_string_append_printf(pattern,
                         "Initializing MongoDB destination; uri='mongodb://%s', db='%s', collection='%s'",
                         uri, db, coll);
  _expect_text_in_log(testcase, pattern->str);
  g_string_free(pattern, TRUE);
}

static void
_test_uri_error(void)
{
  afmongodb_dd_set_uri(mongodb, "INVALID-URI");
  _expect_text_in_log("invalid_uri", "Error parsing MongoDB URI; uri='INVALID-URI'");

  afmongodb_dd_set_uri(mongodb, "mongodb://127.0.0.1:27017/");
  _expect_text_in_log("missing_db", "Missing DB name from MongoDB URI; uri='mongodb://127.0.0.1:27017/'");
}

static void
_test_uri_correct(void)
{
  _expect_uri_in_log("default_uri", "127.0.0.1:27017/syslog" DEFAULTOPTS, "syslog", "messages");

  afmongodb_dd_set_uri(mongodb, "mongodb:///tmp/mongo.sock");
  _expect_uri_in_log("socket", "/tmp/mongo.sock", "tmp/mongo.sock", "messages");

  afmongodb_dd_set_uri(mongodb, "mongodb://localhost:1234/syslog-ng");
  _expect_uri_in_log("uri", "localhost:1234/syslog-ng", "syslog-ng", "messages");

  afmongodb_dd_set_collection(mongodb, "messages2");
  _expect_uri_in_log("collection", "127.0.0.1:27017/syslog" DEFAULTOPTS, "syslog", "messages2");
}

static void
_test_legacy_error(void)
{
  afmongodb_dd_set_safe_mode(mongodb, FALSE);
  afmongodb_dd_set_uri(mongodb, "mongodb://127.0.0.1:27017/syslog");
  _expect_text_in_log("uri_safe_mode", "Error: either specify a MongoDB URI "
                      "(and optional collection) or only legacy options;");
}

static void
_test_legacy_correct(void)
{
  GList *servers = g_list_append(NULL, g_strdup("127.0.0.2:27018"));
  servers = g_list_append(servers, g_strdup("localhost:1234"));
  afmongodb_dd_set_servers(mongodb, servers);
  _expect_uri_in_log("servers_multi", "localhost:1234,127.0.0.2:27018/syslog" DEFAULTOPTS,
                     "syslog", "messages");

  servers = g_list_append(NULL, g_strdup("127.0.0.2"));
  afmongodb_dd_set_servers(mongodb, servers);
  _expect_uri_in_log("servers_single", "127.0.0.2:27017/syslog" DEFAULTOPTS, "syslog", "messages");

  afmongodb_dd_set_host(mongodb, "localhost");
  _expect_uri_in_log("host", "localhost:27017/syslog" DEFAULTOPTS, "syslog", "messages");

  afmongodb_dd_set_host(mongodb, "localhost");
  afmongodb_dd_set_port(mongodb, 1234);
  _expect_uri_in_log("host_port", "localhost:1234/syslog" DEFAULTOPTS, "syslog", "messages");

  afmongodb_dd_set_port(mongodb, 27017);
  _expect_uri_in_log("port_default", "127.0.0.1:27017/syslog" DEFAULTOPTS, "syslog", "messages");

  afmongodb_dd_set_port(mongodb, 1234);
  _expect_uri_in_log("port", "127.0.0.1:1234/syslog" DEFAULTOPTS, "syslog", "messages");

  afmongodb_dd_set_path(mongodb, "/tmp/mongo.sock");
  _expect_uri_in_log("path", "/tmp/mongo.sock" DEFAULTOPTS, "tmp/mongo.sock", "messages");

  afmongodb_dd_set_database(mongodb, "syslog-ng");
  _expect_uri_in_log("database", "127.0.0.1:27017/syslog-ng" DEFAULTOPTS, "syslog-ng", "messages");

  afmongodb_dd_set_safe_mode(mongodb, TRUE);
  _expect_uri_in_log("safe_mode_true", "127.0.0.1:27017/syslog" DEFAULTOPTS, "syslog", "messages");

  afmongodb_dd_set_safe_mode(mongodb, FALSE);
  _expect_uri_in_log("safe_mode_false", "127.0.0.1:27017/syslog", "syslog", "messages");

  afmongodb_dd_set_user(mongodb, "user");
  afmongodb_dd_set_password(mongodb, "password");
  _expect_uri_in_log("user_password", "user:password@127.0.0.1:27017/syslog" DEFAULTOPTS, "syslog", "messages");

  afmongodb_dd_set_collection(mongodb, "messages2");
  afmongodb_dd_set_safe_mode(mongodb, FALSE);
  _expect_uri_in_log("collection_safe_mode", "127.0.0.1:27017/syslog", "syslog", "messages2");
}

static void
_setup(void)
{
  msg_init(FALSE);
  syntax_only = FALSE;
  debug_flag = TRUE;
  verbose_flag = TRUE;
  trace_flag = TRUE;

  log_msg_registry_init();

  test_cfg = cfg_new(0x0308);
  g_assert(test_cfg);

  const gchar *persist_filename = "";
  test_cfg->state = persist_state_new(persist_filename);

  _before_test();
}

static void
_teardown(void)
{
  _free_test();
  if (test_cfg->persist)
    {
      persist_config_free(test_cfg->persist);
      test_cfg->persist = NULL;
    }
  cfg_free(test_cfg);

  log_msg_registry_deinit();
  msg_deinit();
}

int
main(int argc, char **argv)
{
  _setup();

  _test_uri_correct();
  _test_uri_error();

  _test_legacy_correct();
  _test_legacy_error();

  _teardown();
  return (int) _test_ret_num;
}
