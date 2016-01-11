#include "diskq.h"
#include "apphook.h"
#include "plugin.h"
#include "testutils.h"
#include "config_parse_lib.h"
#include "cfg-grammar.h"

#define MEGABYTE 1024*1024

static LogDestDriver *test_driver;
static DiskQDestPlugin *test_plugin;

#define DISKQ_TESTCASE(x, ...) do { diskq_testcase_begin(#x, #__VA_ARGS__); x(__VA_ARGS__); diskq_testcase_end(); } while(0)

#define diskq_testcase_begin(func, args)                        \
  do                                                            \
    {                                                           \
      testcase_begin("%s(%s)", func, args);                     \
      test_driver = g_new0(LogDestDriver, 1);                   \
      log_dest_driver_init_instance(test_driver, configuration);\
      start_grabbing_messages();                                \
    }                                                           \
  while (0)

#define diskq_testcase_end()                                    \
  do                                                            \
    {                                                           \
      log_pipe_deinit((LogPipe *)test_driver);                  \
      log_pipe_unref((LogPipe *)test_driver);                   \
      test_driver = NULL;                                       \
      test_plugin = NULL;                                       \
      testcase_end();                                           \
    }                                                           \
  while (0)

#define test_parse_config(cfg) \
    assert_true(parse_config(cfg, LL_CONTEXT_INNER_DEST, NULL, (gpointer *) &test_plugin), ASSERTION_ERROR("Parsing the given configuration failed")); \
    log_driver_add_plugin(&test_driver->super, (LogDriverPlugin *)test_plugin); \
    assert_true(log_pipe_init(&test_driver->super.super), ASSERTION_ERROR("Failed to init driver"));

static void
check_disk_buf_size_set_to_minimum(gchar *config_to_parse)
{
  test_parse_config(config_to_parse);
  assert_gint64(test_plugin->disk_buf_size, MEGABYTE, ASSERTION_ERROR(NULL));
}

static void
check_disk_buf_size_is_set(gchar *config_to_parse, gint64 value)
{
  test_parse_config(config_to_parse);
  assert_gint64(test_plugin->disk_buf_size, value, ASSERTION_ERROR(NULL));
}

static void
check_mem_buf_size_is_set(gchar *config_to_parse, gint value)
{
  test_parse_config(config_to_parse);
  assert_gint(test_plugin->mem_buf_size, value, ASSERTION_ERROR(NULL));
}

static void
check_mem_buf_length_set_to_minimum(gchar *config_to_parse)
{
  test_parse_config(config_to_parse);
  assert_gint(test_plugin->mem_buf_length, configuration->log_fifo_size, ASSERTION_ERROR(NULL));
}

static void
check_mem_buf_length_is_set(gchar *config_to_parse, gint value)
{
  test_parse_config(config_to_parse);
  assert_gint(test_plugin->mem_buf_length, value, ASSERTION_ERROR(NULL));
}

static void
check_log_qout_size_set_to_minimum(gchar *config_to_parse)
{
  test_parse_config(config_to_parse);
  assert_gint(test_plugin->qout_size, 64, ASSERTION_ERROR(NULL));
}

static void
check_log_qout_size_is_set(gchar *config_to_parse, gint value)
{
  test_parse_config(config_to_parse);
  assert_gint(test_plugin->qout_size, value, ASSERTION_ERROR(NULL));
}

static void
testcase_check_disk_buf_size_is_set()
{
  DISKQ_TESTCASE(check_disk_buf_size_is_set, "disk-buffer(disk-buf-size(1048577))", 1048577);

  DISKQ_TESTCASE(check_disk_buf_size_is_set, "disk-buffer(0)", 0);
  DISKQ_TESTCASE(check_disk_buf_size_is_set, "disk-buffer(1048577)", 1048577);

  DISKQ_TESTCASE(check_disk_buf_size_is_set, "disk-buffer(111 disk-buf-size(1048577))", 1048577);
}

static void
testcase_check_disk_buf_size_set_to_minimum()
{
  DISKQ_TESTCASE(check_disk_buf_size_set_to_minimum, "disk-buffer(disk-buf-size(-2))");
  DISKQ_TESTCASE(check_disk_buf_size_set_to_minimum, "disk-buffer(disk-buf-size(-1))");
  DISKQ_TESTCASE(check_disk_buf_size_set_to_minimum, "disk-buffer(disk-buf-size(0))");
  DISKQ_TESTCASE(check_disk_buf_size_set_to_minimum, "disk-buffer(disk-buf-size(1))");
  DISKQ_TESTCASE(check_disk_buf_size_set_to_minimum, "disk-buffer(disk-buf-size(1048576))");

  DISKQ_TESTCASE(check_disk_buf_size_set_to_minimum, "disk-buffer(-2)");
  DISKQ_TESTCASE(check_disk_buf_size_set_to_minimum, "disk-buffer(-1)");
  DISKQ_TESTCASE(check_disk_buf_size_set_to_minimum, "disk-buffer(1)");
  DISKQ_TESTCASE(check_disk_buf_size_set_to_minimum, "disk-buffer(1048576)");

  DISKQ_TESTCASE(check_disk_buf_size_set_to_minimum, "disk-buffer(111 disk-buf-size(-2))");
  DISKQ_TESTCASE(check_disk_buf_size_set_to_minimum, "disk-buffer(111 disk-buf-size(-1))");
  DISKQ_TESTCASE(check_disk_buf_size_set_to_minimum, "disk-buffer(111 disk-buf-size(0))");
  DISKQ_TESTCASE(check_disk_buf_size_set_to_minimum, "disk-buffer(111 disk-buf-size(1))");
  DISKQ_TESTCASE(check_disk_buf_size_set_to_minimum, "disk-buffer(111 disk-buf-size(1048576))");
}

static void
testcase_check_mem_buf_size_is_set()
{
  DISKQ_TESTCASE(check_mem_buf_size_is_set, "disk-buffer(disk-buf-size(1048576) mem-buf-size(-2))", -2);
  DISKQ_TESTCASE(check_mem_buf_size_is_set, "disk-buffer(disk-buf-size(1048576) mem-buf-size(-1))", -1);
  DISKQ_TESTCASE(check_mem_buf_size_is_set, "disk-buffer(disk-buf-size(1048576) mem-buf-size(0))", 0);
  DISKQ_TESTCASE(check_mem_buf_size_is_set, "disk-buffer(disk-buf-size(1048576) mem-buf-size(1))", 1);
  DISKQ_TESTCASE(check_mem_buf_size_is_set, "disk-buffer(disk-buf-size(1048576) mem-buf-size(1048576))", 1048576);
  DISKQ_TESTCASE(check_mem_buf_size_is_set, "disk-buffer(disk-buf-size(1048576) mem-buf-size(1048577))", 1048577);
}

static void
testcase_check_mem_buf_length_set_to_minimum()
{
  DISKQ_TESTCASE(check_mem_buf_length_set_to_minimum, "disk-buffer(disk-buf-size(1048576) mem-buf-length(-2))");
  DISKQ_TESTCASE(check_mem_buf_length_set_to_minimum, "disk-buffer(disk-buf-size(1048576) mem-buf-length(-1))");
}

static void
testcase_check_mem_buf_length_is_set()
{
  DISKQ_TESTCASE(check_mem_buf_length_is_set, "disk-buffer(disk-buf-size(1048576) mem-buf-length(0))", 0);
  DISKQ_TESTCASE(check_mem_buf_length_is_set, "disk-buffer(disk-buf-size(1048576) mem-buf-length(1))", 1);
}

static void
testcase_check_log_qout_size_set_to_minimum()
{
  DISKQ_TESTCASE(check_log_qout_size_set_to_minimum, "disk-buffer(disk-buf-size(1048576) qout-size(-2))");
  DISKQ_TESTCASE(check_log_qout_size_set_to_minimum, "disk-buffer(disk-buf-size(1048576) qout-size(-1))");
  DISKQ_TESTCASE(check_log_qout_size_set_to_minimum, "disk-buffer(disk-buf-size(1048576) qout-size(0))");
  DISKQ_TESTCASE(check_log_qout_size_set_to_minimum, "disk-buffer(disk-buf-size(1048576) qout-size(1))");
  DISKQ_TESTCASE(check_log_qout_size_set_to_minimum, "disk-buffer(disk-buf-size(1048576) qout-size(64))");
}
static void
testcase_check_log_qout_size_is_set()
{
  DISKQ_TESTCASE(check_log_qout_size_is_set, "disk-buffer(disk-buf-size(1048576) qout-size(65))", 65);
}

int
main()
{
#if _AIX
  fprintf(stderr,"On AIX this testcase can't executed, because the overriding of main_loop_io_worker_register_finish_callback does not work\n");
  return 0;
#endif
  app_startup();
  putenv("TZ=MET-1METDST");
  tzset();

  configuration = cfg_new(0x0308);
  plugin_load_module("syslogformat", configuration, NULL);
  plugin_load_module("disk-buffer", configuration, NULL);

  start_grabbing_messages();

  testcase_check_disk_buf_size_is_set();
  testcase_check_disk_buf_size_set_to_minimum();
  testcase_check_mem_buf_size_is_set();
  testcase_check_mem_buf_length_set_to_minimum();
  testcase_check_mem_buf_length_is_set();
  testcase_check_log_qout_size_set_to_minimum();
  testcase_check_log_qout_size_is_set();

  stop_grabbing_messages();
  return 0;
}
