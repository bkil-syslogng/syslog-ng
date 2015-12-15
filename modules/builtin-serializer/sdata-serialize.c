#include "sdata-serialize.h"

static gboolean
_read_sd_param(SerializeArchive *sa, gchar *sd_element_name, LogMessage *self, gboolean *has_more)
{
  gchar *name = NULL, *value = NULL;
  gsize name_len, value_len;
  gchar sd_param_name[256]={0};
  gboolean success = FALSE;

  if (!serialize_read_cstring(sa, &name, &name_len) ||
      !serialize_read_cstring(sa, &value, &value_len))
    goto error;

  if (name_len != 0 && value_len != 0)
    {
      strcpy(sd_param_name, sd_element_name);
      strncpy(sd_param_name + strlen(sd_element_name), name, name_len);
      log_msg_set_value(self, log_msg_get_value_handle(sd_param_name), value, value_len);
      *has_more = TRUE;
    }
  else
    {
      *has_more = FALSE;
    }

  success = TRUE;
 error:
  g_free(name);
  g_free(value);
  return success;
}

static gboolean
_sd_param_first(SerializeArchive *sa, gchar *sd_element_name, LogMessage *self, gboolean *has_more)
{
  gchar *name, *value;
  gsize name_len, value_len;
  gchar sd_param_name[256]={0};
  gboolean success = FALSE;

  if (!serialize_read_cstring(sa, &name, &name_len) ||
      !serialize_read_cstring(sa, &value, &value_len))
    goto error;

  if (name_len != 0 && value_len != 0)
    {
      strcpy(sd_param_name, sd_element_name);
      strncpy(sd_param_name + strlen(sd_element_name), name, name_len);
      log_msg_set_value(self, log_msg_get_value_handle(sd_param_name), value, value_len);
      *has_more = TRUE;
    }
  else
    {
      *has_more = FALSE;
    }
  success = TRUE;
 error:
  g_free(name);
  g_free(value);
  return success;
}

static gboolean
_read_sd_element(SerializeArchive *sa, LogMessage *self, gboolean *has_more)
{
  gchar *sd_id = NULL;
  gsize sd_id_len;
  gchar sd_element_root[66]={0};
  gboolean has_more_param = TRUE;

  if (!serialize_read_cstring(sa, &sd_id, &sd_id_len))
    return FALSE;

  if (sd_id_len == 0)
    {
      *has_more=FALSE;
      g_free(sd_id);
      return TRUE;
    }
  strcpy(sd_element_root,logmsg_sd_prefix);
  strncpy(sd_element_root + logmsg_sd_prefix_len, sd_id, sd_id_len);
  sd_element_root[logmsg_sd_prefix_len + sd_id_len]='.';

  if (!_sd_param_first(sa, sd_element_root, self, &has_more_param))
    goto error;

  while (has_more_param)
    {
      if (!_read_sd_param(sa, sd_element_root, self, &has_more_param))
        goto error;
    }
  g_free(sd_id);
  *has_more = TRUE;
  return TRUE;

 error:
  g_free(sd_id);
  return FALSE;

}

gboolean
sdata_deserialize(LogMessage *msg, SerializeArchive *sa)
{
  gboolean has_more_element = TRUE;
  if (!_read_sd_element(sa, msg, &has_more_element))
    goto error;


  while (has_more_element)
    {
      if (!_read_sd_element(sa, msg, &has_more_element))
        goto error;
    }
  return TRUE;
 error:

  return FALSE;
}

