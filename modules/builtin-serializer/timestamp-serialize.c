#include "timestamp-serialize.h"

static gboolean
_write_log_stamp(SerializeArchive *sa, LogStamp *stamp)
{
  return serialize_write_uint64(sa, stamp->tv_sec) &&
         serialize_write_uint32(sa, stamp->tv_usec) &&
         serialize_write_uint32(sa, stamp->zone_offset);
}

static gboolean
_read_log_stamp(SerializeArchive *sa, LogStamp *stamp)
{
  guint64 val64;
  guint32 val;

  if (!serialize_read_uint64(sa, &val64))
    return FALSE;
  stamp->tv_sec = (gint64) val64;

  if (!serialize_read_uint32(sa, &val))
    return FALSE;
  stamp->tv_usec = val;

  if (!serialize_read_uint32(sa, &val))
    return FALSE;
  stamp->zone_offset = (gint) val;
  return TRUE;
}


gboolean
timestamp_serialize(SerializeArchive *sa, LogStamp *timestamps)
{
  return _write_log_stamp(sa, &timestamps[LM_TS_STAMP]) &&
         _write_log_stamp(sa, &timestamps[LM_TS_RECVD]) &&
         _write_log_stamp(sa, &timestamps[LM_TS_PROCESSED]); 
}


gboolean
timestamp_deserialize(SerializeArchive *sa, LogStamp *timestamps)
{
  return _read_log_stamp(sa, &timestamps[LM_TS_STAMP]) &&
         _read_log_stamp(sa, &timestamps[LM_TS_RECVD]) &&
         _read_log_stamp(sa, &timestamps[LM_TS_PROCESSED]);
}
