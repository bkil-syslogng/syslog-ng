/*
 * log_queue_disk_non_reliable.c
 *
 *  Created on: Sep 14, 2014
 *      Author: jviktor
 */

#include "logqueue_disk_non_reliable.h"
#include "logpipe.h"
#include "messages.h"
#include "syslog-ng.h"

#define ITEM_NUMBER_PER_MESSAGE 2

typedef struct _LogQueueDiskNR
{
  LogQueueDisk super;
  GQueue *qout;
  GQueue *qoverflow;
  GQueue *qbacklog;
  gint qoverflow_size;
  gint qout_size;
} LogQueueDiskNR;

static gboolean
__start(LogQueueDisk *s, const gchar *filename)
{
  LogQueueDiskNR *self = (LogQueueDiskNR *) s;
  return qdisk_start(s->qdisk, filename, self->qout, self->qbacklog, self->qoverflow);
}

static inline guint
__get_message_number_in_queue(GQueue *queue)
{
  return queue->length / ITEM_NUMBER_PER_MESSAGE;
}

#define HAS_SPACE_IN_QUEUE(queue) __get_message_number_in_queue(queue) < queue ## _size

static gint64
__get_length (LogQueueDisk *s)
{
  LogQueueDiskNR *self = (LogQueueDiskNR *) s;
  return __get_message_number_in_queue(self->qout)
         + qdisk_get_length (s->qdisk)
         + __get_message_number_in_queue(self->qoverflow);
}

static LogMessage *
__get_next_message(LogQueueDiskNR *self, LogPathOptions *path_options)
{
  LogMessage *result = NULL;
  path_options->ack_needed = TRUE;
  if (qdisk_get_length (self->super.qdisk) > 0)
    {
      result = self->super.read_message(&self->super, path_options);
      path_options->ack_needed = FALSE;
    }
  else if (self->qoverflow->length > 0)
    {
      result = g_queue_pop_head (self->qoverflow);
      POINTER_TO_LOG_PATH_OPTIONS (g_queue_pop_head (self->qoverflow), path_options);
    }
  return result;
}

static inline gboolean
__could_move_into_qout(LogQueueDiskNR *self)
{
  /* NOTE: we only load half the qout queue at a time */
  return (__get_message_number_in_queue(self->qout) < (self->qout_size / 2));
}

static void
__add_message_to_qout(LogQueueDiskNR *self, LogMessage *msg, LogPathOptions *path_options)
{
  /* NOTE: we always generate flow-control disabled entries into
   * qout, they only get there via backlog rewind */

  g_queue_push_tail (self->qout, msg);
  g_queue_push_tail (self->qout, LOG_PATH_OPTIONS_FOR_BACKLOG);
  log_msg_ack (msg, path_options, AT_PROCESSED);
}

static inline gboolean
__has_movable_message(LogQueueDiskNR *self)
{
  return self->qoverflow->length > 0
      && ((HAS_SPACE_IN_QUEUE(self->qout) && qdisk_get_length (self->super.qdisk) == 0)
          || qdisk_is_space_avail (self->super.qdisk, 4096));
}

static void
__move_messages_from_overflow(LogQueueDiskNR *self)
{
  LogMessage *msg;
  LogPathOptions path_options;
  /* move away as much entries from the overflow area as possible */
  while (__has_movable_message(self))
    {
      msg = g_queue_pop_head (self->qoverflow);
      POINTER_TO_LOG_PATH_OPTIONS (g_queue_pop_head (self->qoverflow), &path_options);

      if (qdisk_get_length (self->super.qdisk) == 0 && HAS_SPACE_IN_QUEUE(self->qout))
        {
          /* we can skip qdisk, go straight to qout */
          g_queue_push_tail (self->qout, msg);
          g_queue_push_tail (self->qout, LOG_PATH_OPTIONS_FOR_BACKLOG);
          log_msg_ref (msg);
        }
      else
        {
          if (!self->super.write_message(&self->super, msg))
            {
              /* oops, altough there seemed to be some free space available,
               * we failed saving this message, (it might have needed more
               * than 4096 bytes than we ensured), push back and break
               */
              g_queue_push_head (self->qoverflow, LOG_PATH_OPTIONS_TO_POINTER (&path_options));
              g_queue_push_head (self->qoverflow, msg);
              log_msg_ref (msg);
              break;
            }
        }
      log_msg_ack (msg, &path_options, AT_PROCESSED);
      log_msg_unref (msg);
    }
}

static void
__move_disk (LogQueueDiskNR *self)
{
  LogMessage *msg;
  LogPathOptions path_options = LOG_PATH_OPTIONS_INIT;

  if (qdisk_is_read_only (self->super.qdisk))
    return;

  /* stupid message mover between queues */

  if (self->qout->length == 0 && self->qout_size > 0)
    {
      do
        {
          msg = __get_next_message(self, &path_options);

          if (msg)
            {
              __add_message_to_qout(self, msg, &path_options);
            }
        }
      while (msg && __could_move_into_qout(self));
    }
  __move_messages_from_overflow(self);
}

static void
__ack_backlog (LogQueueDisk *s, guint num_msg_to_ack)
{
  LogQueueDiskNR *self = (LogQueueDiskNR *) s;
  LogMessage *msg;
  LogPathOptions path_options = LOG_PATH_OPTIONS_INIT;
  guint i;

  for (i = 0; i < num_msg_to_ack; i++)
    {
      if (self->qbacklog->length < ITEM_NUMBER_PER_MESSAGE)
        return;
      msg = g_queue_pop_head (self->qbacklog);
      POINTER_TO_LOG_PATH_OPTIONS (g_queue_pop_head (self->qbacklog), &path_options);
      log_msg_unref (msg);
      log_msg_ack (msg, &path_options, AT_PROCESSED);
    }
}

static void
__rewind_backlog (LogQueueDisk *s, guint rewind_count)
{
  guint i;
  LogQueueDiskNR *self = (LogQueueDiskNR *) s;

  rewind_count = MIN(rewind_count, __get_message_number_in_queue(self->qbacklog));

  for (i = 0; i < rewind_count; i++)
    {
      gpointer ptr_opt = g_queue_pop_tail (self->qbacklog);
      gpointer ptr_msg = g_queue_pop_tail (self->qbacklog);

      g_queue_push_head (self->qout, ptr_opt);
      g_queue_push_head (self->qout, ptr_msg);

      stats_counter_inc (self->super.super.stored_messages);
    }
}

static LogMessage *
__pop_head (LogQueueDisk *s, LogPathOptions *path_options)
{
  LogQueueDiskNR *self = (LogQueueDiskNR *) s;
  LogMessage *msg = NULL;

  if (self->qout->length > 0)
    {
      msg = g_queue_pop_head (self->qout);
      POINTER_TO_LOG_PATH_OPTIONS (g_queue_pop_head (self->qout), path_options);
    }
  if (msg == NULL)
    {
      msg = s->read_message(s, path_options);
      if (msg)
        {
          path_options->ack_needed = FALSE;
        }
    }
  if (msg == NULL)
    {
      if (self->qoverflow->length > 0 && qdisk_is_read_only (self->super.qdisk))
        {
          msg = g_queue_pop_head (self->qoverflow);
          POINTER_TO_LOG_PATH_OPTIONS (g_queue_pop_head (self->qoverflow), path_options);
        }
    }

  if (msg != NULL)
    {
      if (self->super.super.use_backlog)
        {
          log_msg_ref (msg);
          g_queue_push_tail (self->qbacklog, msg);
          g_queue_push_tail (self->qbacklog, LOG_PATH_OPTIONS_TO_POINTER (path_options));
        }
      __move_disk (self);
    }
  return msg;
}

static void
__push_head (LogQueueDisk *s, LogMessage *msg, const LogPathOptions *path_options)
{
  LogQueueDiskNR *self = (LogQueueDiskNR *) s;

  g_static_mutex_lock(&self->super.super.lock);
  g_queue_push_head (self->qout, LOG_PATH_OPTIONS_TO_POINTER (path_options));
  g_queue_push_head (self->qout, msg);
  stats_counter_inc (self->super.super.stored_messages);
  g_static_mutex_unlock(&self->super.super.lock);
}

static gboolean
__push_tail (LogQueueDisk *s, LogMessage *msg, LogPathOptions *local_options, const LogPathOptions *path_options)
{
  LogQueueDiskNR *self = (LogQueueDiskNR *) s;

  if (HAS_SPACE_IN_QUEUE(self->qout) && qdisk_get_length (self->super.qdisk) == 0)
    {
      /* simple push never generates flow-control enabled entries to qout, they only get there
       * when rewinding the backlog */

      g_queue_push_tail (self->qout, msg);
      g_queue_push_tail (self->qout, LOG_PATH_OPTIONS_FOR_BACKLOG);
      log_msg_ref (msg);
    }
  else
    {
      if (self->qoverflow->length != 0 || !s->write_message(s, msg))
        {
          if (HAS_SPACE_IN_QUEUE(self->qoverflow))
            {
              g_queue_push_tail (self->qoverflow, msg);
              g_queue_push_tail (self->qoverflow, LOG_PATH_OPTIONS_TO_POINTER (path_options));
              log_msg_ref (msg);
              local_options->ack_needed = FALSE;
            }
          else
            {
              msg_debug ("Destination queue full, dropping message",
                         evt_tag_str ("filename", qdisk_get_filename (self->super.qdisk)),
                         evt_tag_int ("queue_len", __get_length(s)),
                         evt_tag_int ("mem_buf_length", self->qoverflow_size),
                         evt_tag_int ("size", qdisk_get_size (self->super.qdisk)),
                         evt_tag_str ("persist_name", self->super.super.persist_name),
                         NULL);
              return FALSE;
            }
        }
    }
  return TRUE;
}

static void
__free_queue (GQueue *q)
{
  while (!g_queue_is_empty (q))
    {
      LogMessage *lm;
      LogPathOptions path_options = LOG_PATH_OPTIONS_INIT;

      lm = g_queue_pop_head (q);
      POINTER_TO_LOG_PATH_OPTIONS (g_queue_pop_head (q), &path_options);
      log_msg_ack (lm, &path_options, AT_PROCESSED);
      log_msg_unref (lm);
    }
  g_queue_free (q);
}

static void
__freefn (LogQueueDisk *s)
{
  LogQueueDiskNR *self = (LogQueueDiskNR *) s;
  __free_queue (self->qoverflow);
  self->qoverflow = NULL;
  __free_queue (self->qout);
  self->qout = NULL;
  __free_queue (self->qbacklog);
  self->qbacklog = NULL;
}

static gboolean
__load_queue (LogQueueDisk *s, const gchar *filename)
{
  /* qdisk portion is not yet started when this happens */
  g_assert(!qdisk_initialized (s->qdisk));

  return __start(s, filename);
}

static gboolean
__save_queue (LogQueueDisk *s, gboolean *persistent)
{
  LogQueueDiskNR *self = (LogQueueDiskNR *) s;
  if (qdisk_save_state (s->qdisk, self->qout, self->qbacklog, self->qoverflow))
    {
      *persistent = TRUE;
      qdisk_deinit (s->qdisk);
      return TRUE;
    }
  return FALSE;
}

static void
__set_virtual_functions (LogQueueDisk *self)
{
  self->get_length = __get_length;
  self->ack_backlog = __ack_backlog;
  self->rewind_backlog = __rewind_backlog;
  self->pop_head = __pop_head;
  self->push_head = __push_head;
  self->push_tail = __push_tail;
  self->start = __start;
  self->free_fn = __freefn;
  self->load_queue = __load_queue;
  self->save_queue = __save_queue;
}

LogQueue *
log_queue_disk_non_reliable_read_only_new(gint64 qdisk_size, gint qout_size, gint qoverflow_size, LogMsgSerializer *serializer)
{
  LogQueueDiskNR *self = g_new0(LogQueueDiskNR, 1);
  log_queue_disk_init_instance (&self->super);
  qdisk_init (self->super.qdisk, qdisk_size, TRUE, FALSE, 0, serializer);
  self->qbacklog = g_queue_new ();
  self->qout = g_queue_new ();
  self->qoverflow = g_queue_new ();
  self->qout_size = qout_size;
  self->qoverflow_size = qoverflow_size;
  __set_virtual_functions (&self->super);
  return &self->super.super;
}

LogQueue *
log_queue_disk_non_reliable_new (gint64 qdisk_size, gint qout_size, gint qoverflow_size, LogMsgSerializer *serializer)
{
  LogQueueDiskNR *self = g_new0(LogQueueDiskNR, 1);
  log_queue_disk_init_instance (&self->super);
  qdisk_init (self->super.qdisk, qdisk_size, FALSE, FALSE, 0, serializer);
  self->qbacklog = g_queue_new ();
  self->qout = g_queue_new ();
  self->qoverflow = g_queue_new ();
  self->qout_size = qout_size;
  self->qoverflow_size = qoverflow_size;
  __set_virtual_functions (&self->super);
  return &self->super.super;
}
