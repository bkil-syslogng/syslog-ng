#include "testutils.h"
#include "apphook.h"
#include "logmsg.h"
#include "timestamp-serialize.h"

#define PREPARE_TEST LogStamp input_timestamps[LM_TS_MAX]; \
  LogStamp output_timestamps[LM_TS_MAX]; \
  GString *stream = g_string_new(""); \
  input_timestamps[LM_TS_STAMP].tv_sec = 1; \
  input_timestamps[LM_TS_STAMP].tv_usec = 2; \
  input_timestamps[LM_TS_STAMP].zone_offset = 3; \
  input_timestamps[LM_TS_RECVD].tv_sec = 4; \
  input_timestamps[LM_TS_RECVD].tv_usec = 5; \
  input_timestamps[LM_TS_RECVD].zone_offset = 6; \
  input_timestamps[LM_TS_PROCESSED].tv_sec = 7; \
  input_timestamps[LM_TS_PROCESSED].tv_usec = 8; \
  input_timestamps[LM_TS_PROCESSED].zone_offset = 9; \
  SerializeArchive *sa = serialize_string_archive_new(stream);

#define CLEAN_TEST serialize_archive_free(sa); \
  g_string_free(stream, TRUE);

static void
test_normal_working()
{
  PREPARE_TEST
  assert_true(timestamp_serialize(sa, input_timestamps), "Failed to serialize timestamps");
  assert_true(timestamp_deserialize(sa, output_timestamps), "Failed to deserialize timestamps");

  assert_nstring((const gchar *)input_timestamps, sizeof(input_timestamps),
                 (const gchar *)output_timestamps, sizeof(output_timestamps),
                 "The serialized and the deserialized timestamps are not equal");

  CLEAN_TEST
}

static void
test_derializing_injured_timestamp()
{
  PREPARE_TEST

  assert_true(timestamp_serialize(sa, input_timestamps), "Failed to serialize timestamps");

  g_string_truncate(stream, 0);
  assert_false(timestamp_deserialize(sa, output_timestamps), "Should be failed");

  serialize_archive_free(sa);
  sa = serialize_string_archive_new(stream);
  assert_true(timestamp_serialize(sa, input_timestamps), "Failed to serialize timestamps");
  g_string_truncate(stream, sizeof(guint64));
  assert_false(timestamp_deserialize(sa, output_timestamps), "Should be failed");

  serialize_archive_free(sa);
  sa = serialize_string_archive_new(stream);
  assert_true(timestamp_serialize(sa, input_timestamps), "Failed to serialize timestamps");
  g_string_truncate(stream, sizeof(guint64) + sizeof(guint32));
  assert_false(timestamp_deserialize(sa, output_timestamps), "Should be failed");

  CLEAN_TEST
}

int
main(int argc, char **argv)
{
  app_startup();

  test_normal_working();

  start_grabbing_messages();
  test_derializing_injured_timestamp();
  stop_grabbing_messages();

  app_shutdown();
  return 0;
}
