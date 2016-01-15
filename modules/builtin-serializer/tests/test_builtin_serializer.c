#include "testutils.h"
#include "apphook.h"
#include "cfg.h"
#include "plugin.h"
#include "plugin-types.h"
#include "builtin-serializer-plugin.c"
#include "logmsg-serializer.h"

GlobalConfig *cfg;

#define RAW_MSG "<132>1 2006-10-29T01:59:59.156+01:00 mymachine evntslog - - [exampleSDID@0 iut=\"3\" eventSource=\"Application\"] An application event log entry..."

#define ERROR_MSG "Failed at %s(%d)", __FILE__, __LINE__

MsgFormatOptions parse_options;

void
test_serializer()
{
  LogMessage *msg = NULL;
  LogTemplate *template = NULL;
  GString *stream = g_string_new("");
  GString *output = g_string_new("");
  NVHandle test_handle = 0;
  NVHandle indirect_handle = 0;
  SerializeArchive *sa = serialize_string_archive_new(stream);
  GError *error = NULL;
  gssize length = 0;

  LogMsgSerializer *serializer = log_msg_serializer_factory(cfg, "builtin", &error);

  parse_options.flags |= LP_SYSLOG_PROTOCOL;
  test_handle = log_msg_get_value_handle("aaa");

  template = log_template_new(cfg, NULL);
  msg = log_msg_new(RAW_MSG, strlen(RAW_MSG), NULL, &parse_options);
  log_msg_set_value(msg, test_handle, "test_value", -1);
  log_template_compile(template, "${.SDATA.exampleSDID@0.eventSource}", NULL);
  log_template_append_format(template, msg, &cfg->template_options, LTZ_SEND, 0, NULL, output);
  assert_string(output->str, "Application", ERROR_MSG);

  indirect_handle = log_msg_get_value_handle("indirect_1");
  log_msg_set_value_indirect(msg, indirect_handle, test_handle, 0, 5, 3);
  const gchar *indirect_value = log_msg_get_value(msg, indirect_handle, &length);
  assert_nstring(indirect_value, length, "val", 3, ERROR_MSG);

  log_msg_serialize(serializer, msg, sa);
  log_msg_unref(msg);
  log_template_unref(template);
  g_string_free(output, TRUE);
  log_msg_registry_deinit();

  log_msg_registry_init();

  output = g_string_new("");
  template = log_template_new(cfg, NULL);
  log_template_compile(template, "${.SDATA.exampleSDID@0.eventSource}", NULL);
  msg = log_msg_new_empty();

  assert_true(log_msg_deserialize(serializer, msg, sa), ERROR_MSG);
  log_template_append_format(template, msg, &cfg->template_options, LTZ_SEND, 0, NULL, output);
  assert_string(output->str, "Application", ERROR_MSG);

  indirect_handle = log_msg_get_value_handle("indirect_1");
  indirect_value = log_msg_get_value(msg, indirect_handle, &length);
  assert_nstring(indirect_value, length, "val", 3, ERROR_MSG);

  log_msg_unref(msg);
  serialize_archive_free(sa);
  log_template_unref(template);
  log_msg_serializer_free(serializer);
  g_string_free(stream, TRUE);
  g_string_free(output, TRUE);
}

int
main(int argc, char **argv)
{
  app_startup();
  cfg = cfg_new(0x0307);
  builtin_serializer_module_init(cfg, NULL);
  plugin_load_module("syslogformat", cfg, NULL);
  msg_format_options_defaults(&parse_options);
  msg_format_options_init(&parse_options, cfg);
  test_serializer();
  return 0;
}
