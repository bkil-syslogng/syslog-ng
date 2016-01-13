/*
 * logqueue_disk_reliable.c
 *
 *  Created on: Sep 14, 2014
 *      Author: jviktor
 */

#include "syslog-ng.h"
#include "qdisk.h"
#include "logpipe.h"
#include "logqueue_disk_reliable.h"
#include "messages.h"

typedef struct _LogQueueDiskReliable
{
  LogQueueDisk super;
  GQueue *qreliable;
  GQueue *qbacklog;
} LogQueueDiskReliable;

static gboolean
__start(LogQueueDisk *s, const gchar *filename)
{
  return qdisk_start(s->qdisk, filename, NULL, NULL, NULL);
}

static gboolean
__skip_message(LogQueueDisk *self)
{
  GString *serialized;
  SerializeArchive *sa;

  if (!qdisk_initialized(self->qdisk))
    return FALSE;

  serialized = g_string_sized_new(64);
  if (!qdisk_pop_head(self->qdisk, serialized))
    {
      g_string_free(serialized, TRUE);
      return FALSE;
    }

  sa = serialize_string_archive_new(serialized);
  serialize_archive_free(sa);

  g_string_free(serialized, TRUE);
  return TRUE;
}

static void
__empty_queue(GQueue *self)
{
  while (self && self->length > 0)
    {
      LogPathOptions path_options = LOG_PATH_OPTIONS_INIT;

      gint64 *temppos = g_queue_pop_head(self);
      LogMessage *msg = g_queue_pop_head(self);
      POINTER_TO_LOG_PATH_OPTIONS(g_queue_pop_head(self), &path_options);

      g_free(temppos);
      log_msg_drop(msg, &path_options);
    }
}

static gint64
__get_length(LogQueueDisk *self)
{
  return qdisk_get_length(self->qdisk);
}

static void
__ack_backlog(LogQueueDisk *s, guint num_msg_to_ack)
{
  LogQueueDiskReliable *self = (LogQueueDiskReliable *) s;
  LogMessage *msg;
  LogPathOptions path_options = LOG_PATH_OPTIONS_INIT;
  guint i;

  for (i = 0; i < num_msg_to_ack; i++)
    {
      gint64 pos;
      if (qdisk_get_backlog_head (self->super.qdisk) == qdisk_get_reader_head (self->super.qdisk))
        {
          goto exit_reliable;
        }
      if (self->qbacklog->length > 0)
        {
          gint64 *temppos = g_queue_pop_head (self->qbacklog);
          pos = *temppos;
          if (pos == qdisk_get_backlog_head (self->super.qdisk))
            {
              msg = g_queue_pop_head (self->qbacklog);
              POINTER_TO_LOG_PATH_OPTIONS (g_queue_pop_head (self->qbacklog), &path_options);
              log_msg_ack (msg, &path_options, AT_PROCESSED);
              log_msg_unref (msg);
              g_free (temppos);
            }
          else
            {
              g_queue_push_head (self->qbacklog, temppos);
            }
        }
      guint64 new_backlog = qdisk_get_backlog_head (self->super.qdisk);
      new_backlog = qdisk_skip_record(self->super.qdisk, new_backlog);
      qdisk_set_backlog_head (self->super.qdisk, new_backlog);
      qdisk_dec_backlog (self->super.qdisk);
    }
exit_reliable:
  qdisk_reset_file_if_possible (self->super.qdisk);
}

static gint
__find_pos_in_qbacklog(LogQueueDiskReliable *self, gint64 new_pos)
{
  gint result = -1;
  int i = 0;
  GList *item = self->qbacklog->tail;
  while (result == -1 && item != NULL)
    {
      GList *pos_i = item->prev->prev;
      gint64 *pos = pos_i->data;
      if (*pos == new_pos)
        {
          result = i;
        }
      item = pos_i->prev;
      i++;
    }
  return result;
}

static void
__move_message_from_qbacklog_to_qreliable(LogQueueDiskReliable *self)
{
  gpointer ptr_opt = g_queue_pop_tail(self->qbacklog);
  gpointer ptr_msg = g_queue_pop_tail(self->qbacklog);
  gpointer ptr_pos = g_queue_pop_tail(self->qbacklog);

  g_queue_push_head(self->qreliable, ptr_opt);
  g_queue_push_head(self->qreliable, ptr_msg);
  g_queue_push_head(self->qreliable, ptr_pos);
}

static void
__rewind_from_qbacklog(LogQueueDiskReliable *self, gint64 new_pos)
{
  gint i;
  g_assert((self->qbacklog->length % 3) == 0);

  gint rewind_backlog_queue = __find_pos_in_qbacklog(self, new_pos);

  for (i = 0; i <= rewind_backlog_queue; i++)
    {
      __move_message_from_qbacklog_to_qreliable(self);
    }
}


static void
__rewind_backlog(LogQueueDisk *s, guint rewind_count)
{
  guint i;

  guint number_of_messages_stay_in_backlog;
  gint64 new_read_head;
  LogQueueDiskReliable *self = (LogQueueDiskReliable *) s;

  rewind_count = MIN(rewind_count, qdisk_get_backlog_count (self->super.qdisk));
  number_of_messages_stay_in_backlog = qdisk_get_backlog_count (self->super.qdisk) - rewind_count;
  new_read_head = qdisk_get_backlog_head (self->super.qdisk);
  for (i = 0; i < number_of_messages_stay_in_backlog; i++)
    {
      new_read_head = qdisk_skip_record(self->super.qdisk, new_read_head);
    }

  __rewind_from_qbacklog(self, new_read_head);

  qdisk_set_backlog_count (self->super.qdisk, number_of_messages_stay_in_backlog);
  qdisk_set_reader_head (self->super.qdisk, new_read_head);
  qdisk_set_length (self->super.qdisk, qdisk_get_length (self->super.qdisk) + rewind_count);

  stats_counter_add (self->super.super.stored_messages, rewind_count);
}

static LogMessage *
__pop_head(LogQueueDisk *s, LogPathOptions *path_options)
{
  LogQueueDiskReliable *self = (LogQueueDiskReliable *) s;
  LogMessage *msg = NULL;
  if (self->qreliable->length > 0)
    {
      gint64 *temppos = g_queue_pop_head (self->qreliable);
      gint64 pos = *temppos;
      if (pos == qdisk_get_reader_head (self->super.qdisk))
        {
          msg = g_queue_pop_head (self->qreliable);
          POINTER_TO_LOG_PATH_OPTIONS (g_queue_pop_head (self->qreliable), path_options);
          __skip_message (s);
          if (self->super.super.use_backlog)
            {
              log_msg_ref (msg);
              g_queue_push_tail (self->qbacklog, temppos);
              g_queue_push_tail (self->qbacklog, msg);
              g_queue_push_tail (self->qbacklog, LOG_PATH_OPTIONS_TO_POINTER (path_options));
            }
          else
            {
              g_free (temppos);
            }
        }
      else
        {
          /* try to write the message to the disk */
          g_queue_push_head (self->qreliable, temppos);
        }
    }

  if (msg == NULL)
    {
      msg = s->read_message(s, path_options);
      if (msg)
        {
          path_options->ack_needed = FALSE;
        }
    }

  if (msg != NULL)
    {
      if (self->super.super.use_backlog)
        {
          qdisk_inc_backlog(self->super.qdisk);
        }
      else
        {
          qdisk_set_backlog_head(self->super.qdisk, qdisk_get_reader_head(self->super.qdisk));
        }
    }
  return msg;
}

static gboolean
__push_tail(LogQueueDisk *s, LogMessage *msg, LogPathOptions *local_options, const LogPathOptions *path_options)
{
  LogQueueDiskReliable *self = (LogQueueDiskReliable *) s;

  gint64 last_wpos = qdisk_get_writer_head (self->super.qdisk);
  if (!s->write_message(s, msg))
    {
      /* we were not able to store the msg, warn */
      msg_error("Destination reliable queue full, dropping message",
                evt_tag_str("filename", qdisk_get_filename (self->super.qdisk)),
                evt_tag_int("queue_len", __get_length(s)),
                evt_tag_int("mem_buf_size", qdisk_get_memory_size (self->super.qdisk)),
                evt_tag_int("disk_buf_size", qdisk_get_size (self->super.qdisk)),
                evt_tag_str("persist_name", self->super.super.persist_name), NULL);

      return FALSE;
    }

  /* check the remaining space: if it is less than the mem_buf_size, the message cannot be acked */
  gint64 wpos = qdisk_get_writer_head (self->super.qdisk);
  gint64 bpos = qdisk_get_backlog_head (self->super.qdisk);
  gint64 diff;
  if (wpos > bpos)
    diff = qdisk_get_size (self->super.qdisk) - wpos + bpos - QDISK_RESERVED_SPACE;
  else
    diff = bpos - wpos;
  gboolean overflow = diff < qdisk_get_memory_size (self->super.qdisk);

  if (overflow)
    {
      /* we have reached the reserved buffer size, keep the msg in memory
       * the message is written but into the overflow area
       */
      gint64 *temppos = g_malloc (sizeof(gint64));
      *temppos = last_wpos;
      g_queue_push_tail (self->qreliable, temppos);
      g_queue_push_tail (self->qreliable, msg);
      g_queue_push_tail (self->qreliable, LOG_PATH_OPTIONS_TO_POINTER(path_options));
      log_msg_ref (msg);

      local_options->ack_needed = FALSE;
    }

  return TRUE;
}

static void
__free_queue(LogQueueDisk *s)
{
  LogQueueDiskReliable *self = (LogQueueDiskReliable *) s;
  __empty_queue(self->qreliable);
  __empty_queue(self->qbacklog);
  g_queue_free(self->qreliable);
  self->qreliable = NULL;
  g_queue_free(self->qbacklog);
  self->qbacklog = NULL;
}

static gboolean
__load_queue(LogQueueDisk *s, const gchar *filename)
{
  LogQueueDiskReliable *self = (LogQueueDiskReliable *) s;
  __empty_queue(self->qreliable);
  return qdisk_start(s->qdisk, filename, NULL, NULL, NULL);
}

static gboolean
__save_queue (LogQueueDisk *s, gboolean *persistent)
{
  *persistent = TRUE;
  qdisk_deinit (s->qdisk);
  return TRUE;
}


static void
__set_virtual_functions(LogQueueDisk *self)
{
  self->get_length = __get_length;
  self->ack_backlog = __ack_backlog;
  self->rewind_backlog = __rewind_backlog;
  self->pop_head = __pop_head;
  self->push_tail = __push_tail;
  self->free_fn = __free_queue;
  self->load_queue = __load_queue;
  self->start = __start;
  self->save_queue = __save_queue;
}

LogQueue *
log_queue_disk_reliable_read_only_new(gint64 qdisk_size, gint mem_buf_size, LogMsgSerializer *serializer, const gchar *dir)
{
  LogQueueDiskReliable *self = g_new0(LogQueueDiskReliable, 1);
  log_queue_disk_init_instance(&self->super);
  qdisk_init(self->super.qdisk, qdisk_size, TRUE, TRUE, mem_buf_size, serializer, dir);
  self->qreliable = g_queue_new();
  self->qbacklog = g_queue_new();
  __set_virtual_functions(&self->super);
  return &self->super.super;
}

LogQueue *
log_queue_disk_reliable_new(gint64 qdisk_size, gint mem_buf_size, LogMsgSerializer *serializer, const gchar *dir)
{
  LogQueueDiskReliable *self = g_new0(LogQueueDiskReliable, 1);
  log_queue_disk_init_instance(&self->super);
  qdisk_init(self->super.qdisk, qdisk_size, FALSE, TRUE, mem_buf_size, serializer, dir);
  self->qreliable = g_queue_new();
  self->qbacklog = g_queue_new();
  __set_virtual_functions(&self->super);
  return &self->super.super;
}
