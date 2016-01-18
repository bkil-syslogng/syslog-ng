#include "messages.h"
#include "logqueue-disk.h"
#include "logqueue-disk-reliable.h"
#include "logqueue-disk-non-reliable.h"
#include "apphook.h"
#include "plugin.h"
#include "testutils.h"
#include "queue_utils_lib.h"
#include "test_diskq_tools.h"

#include <stdlib.h>
#include <string.h>

#define DISKQ_FILENAME "test_become_full.qf"

MsgFormatOptions parse_options;


static void msg_post_function(LogMessage *msg)
{
  log_msg_unref(msg);
}

void
test_diskq_become_full(gboolean reliable)
{
  LogQueue *q;
  acked_messages = 0;
  fed_messages = 0;
  DiskQueueOptions options = {0};

  options.reliable = reliable;
  if (reliable)
    {
      _construct_options(&options, 1000, 1000, reliable);
      q = log_queue_disk_reliable_new(&options);
    }
  else
    {
      _construct_options(&options, 1000, 0, reliable);
      q = log_queue_disk_non_reliable_new(&options);
    }

  log_queue_set_use_backlog(q, TRUE);

  q->persist_name = "test_diskq";
  stats_lock();
  stats_register_counter(0, SCS_DESTINATION, q->persist_name, NULL, SC_TYPE_DROPPED, &q->dropped_messages);
  stats_counter_set(q->dropped_messages, 0);
  stats_unlock();
  unlink(DISKQ_FILENAME);
  log_queue_disk_load_queue(q, DISKQ_FILENAME);
  feed_some_messages(q, 1000, &parse_options);

  assert_gint(q->dropped_messages->value, 1000, "Bad dropped message number (reliable: %s)", reliable ? "TRUE" : "FALSE");

  log_queue_unref(q);
  disk_queue_options_destroy(&options);
  unlink(DISKQ_FILENAME);
}

int
main()
{
  app_startup();
  putenv("TZ=MET-1METDST");
  tzset();

  configuration = cfg_new(0x0308);
  plugin_load_module("syslogformat", configuration, NULL );
  plugin_load_module("disk-buffer", configuration, NULL );
  plugin_load_module("builtin-serializer", configuration, NULL );
  msg_set_post_func(msg_post_function);
  msg_format_options_defaults(&parse_options);
  msg_format_options_init(&parse_options, configuration);
  test_diskq_become_full(TRUE);
  test_diskq_become_full(FALSE);
  return 0;
}
