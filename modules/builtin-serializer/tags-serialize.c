#include "tags-serialize.h"

gboolean
tags_deserialize(LogMessage *msg, SerializeArchive *sa)
{
  gchar *buf;
  gsize len;

  while (1)
    {
      if (!serialize_read_cstring(sa, &buf, &len) || !buf)
        return FALSE;
      if (!buf[0])
        {
          /* "" , empty string means: last tag */
          g_free(buf);
          break;
        }
      log_msg_set_tag_by_name(msg, buf);
      g_free(buf);
    }

  msg->flags |= LF_STATE_OWN_TAGS;

  return TRUE;
}

static gboolean
_callback(const LogMessage *msg, LogTagId tag_id, const gchar *name, gpointer user_data)
{
  SerializeArchive *sa = ( SerializeArchive *)user_data;
  serialize_write_cstring(sa, name, strlen(name));
  return TRUE;
}

gboolean
tags_serialize(LogMessage *msg, SerializeArchive *sa)
{
  log_msg_tags_foreach(msg, _callback, (gpointer)sa);
  return serialize_write_cstring(sa, "", 0);
}


