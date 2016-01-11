#include "persist-state.h"
#include "state.h"
#include "apphook.h"
#include "testutils.h"
#include "plugin.h"

#include "diskq-plugin.c"

#define TEST_PERSIST_FILE "test_sql_state.persist"
#define TEST_NAME "diskq_file"
#define TEST_VALUE "/var/queues/queue.001"

void
test_state_dump()
{
  PersistState *state;
  StateHandler *state_handler;
  NameValueContainer *dumped_values;
  gchar *value;

  unlink(TEST_PERSIST_FILE);
  state = persist_state_new(TEST_PERSIST_FILE);
  assert_true(persist_state_start(state),
      "Error starting persist_state object");

  persist_state_alloc_string(state, TEST_NAME, TEST_VALUE, strlen(TEST_VALUE));
  persist_state_commit(state);
  persist_state_free(state);

  state = persist_state_new(TEST_PERSIST_FILE);
  assert_true(persist_state_start(state),
        "Error starting persist_state object");
  state_handler = disk_buffer_state_handler_new(state, TEST_NAME);
  dumped_values = state_handler_dump_state(state_handler);
  assert_true(dumped_values != NULL, "dump_state returned with NULL");

  value = name_value_container_get_value(dumped_values, "queue_file");
  assert_string(value, TEST_VALUE, "Bad dumped value");
  g_free(value);

  name_value_container_free(dumped_values);
  state_handler_free(state_handler);
  persist_state_commit(state);
  persist_state_free(state);
}

void
test_prefixes()
{
  GlobalConfig *cfg = cfg_new(0x302);
  STATE_HANDLER_CONSTRUCTOR constructor;

  assert_true(plugin_load_module("disk-buffer", cfg, NULL),
      "Can't load plugin");

  constructor = state_handler_get_constructor_by_prefix("afsocket_dd_qfile");
  assert_gpointer(constructor, disk_buffer_state_handler_new,
      "Bad constructor");

  constructor = state_handler_get_constructor_by_prefix("afprogram_dd_qname");
  assert_gpointer(constructor, disk_buffer_state_handler_new,
      "Bad constructor");

  constructor = state_handler_get_constructor_by_prefix("afsql_dd_qfile");
  assert_gpointer(constructor, disk_buffer_state_handler_new,
      "Bad constructor");

  constructor = state_handler_get_constructor_by_prefix("snmpdest");
  assert_gpointer(constructor, disk_buffer_state_handler_new,
      "Bad constructor");

  cfg_free(cfg);
}

int
main(int argc, char **argv)
{
  app_startup();
#ifdef linux
  test_prefixes();
#endif
  test_state_dump();
  return 0;
}
