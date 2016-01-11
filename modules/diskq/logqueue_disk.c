/*
 * Copyright (C) 2006-2011 BalaBit IT Ltd.
 *
 * All rights reserved.
 */
#include "logqueue_disk.h"
#include "logpipe.h"
#include "messages.h"
#include "serialize.h"
#include "logmsg-serializer.h"
#include "stats/stats-registry.h"
#include "reloc.h"
#include "qdisk.h"

#include <sys/types.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <sys/mman.h>
#endif
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

const QueueType log_queue_disk_type = "DISK";

gint64
log_queue_disk_get_length(LogQueue *s)
{
  LogQueueDisk *self = (LogQueueDisk *) s;
  gint64 qdisk_length = 0;

  if (qdisk_initialized(self->qdisk) && self->get_length)
    {
      qdisk_length = self->get_length(self);
    }
  return qdisk_length;
}

static void
log_queue_disk_push_tail(LogQueue *s, LogMessage *msg, const LogPathOptions *path_options)
{
  LogQueueDisk *self = (LogQueueDisk *) s;
  LogPathOptions local_options = *path_options;
  g_static_mutex_lock(&self->super.lock);
  if (self->push_tail)
    {
      if (self->push_tail(self, msg, &local_options, path_options))
        {
          log_queue_push_notify (&self->super);
          stats_counter_inc(self->super.stored_messages);
          log_msg_ack(msg, &local_options, AT_PROCESSED);
          log_msg_unref(msg);
          g_static_mutex_unlock(&self->super.lock);
          return;
        }
    }
  stats_counter_inc (self->super.dropped_messages);

  log_msg_drop(msg, path_options);

  g_static_mutex_unlock(&self->super.lock);
}

void
log_queue_disk_push_head(LogQueue *s, LogMessage *msg, const LogPathOptions *path_options)
{
  LogQueueDisk *self = (LogQueueDisk *) s;

  g_static_mutex_lock(&self->super.lock);
  if (self->push_head)
    {
      self->push_head(self, msg, path_options);
    }
  g_static_mutex_unlock(&self->super.lock);
}

LogMessage *
log_queue_disk_pop_head(LogQueue *s, LogPathOptions *path_options)
{
  LogQueueDisk *self = (LogQueueDisk *) s;
  LogMessage *msg = NULL;

  msg = NULL;
  g_static_mutex_lock(&self->super.lock);
  if (self->pop_head)
    {
      msg = self->pop_head(self, path_options);
    }
  if (msg != NULL)
    {
      stats_counter_dec(self->super.stored_messages);
    }
  g_static_mutex_unlock(&self->super.lock);
  return msg;
}

void
log_queue_disk_ack_backlog(LogQueue *s, gint num_msg_to_ack)
{
  LogQueueDisk *self = (LogQueueDisk *) s;

  g_static_mutex_lock(&self->super.lock);

  if (self->ack_backlog)
    {
      self->ack_backlog(self, num_msg_to_ack);
    }

  g_static_mutex_unlock(&self->super.lock);
}

void
log_queue_disk_rewind_backlog(LogQueue *s, guint rewind_count)
{
  LogQueueDisk *self = (LogQueueDisk *) s;

  g_static_mutex_lock(&self->super.lock);

  if (self->rewind_backlog)
    {
      self->rewind_backlog (self, rewind_count);
    }

  g_static_mutex_unlock(&self->super.lock);
}

void
log_queue_disk_rewind_backlog_all(LogQueue *s)
{
  LogQueueDisk *self = (LogQueueDisk *) s;

  g_static_mutex_lock(&self->super.lock);

  if (self->rewind_backlog)
    {
      self->rewind_backlog(self, -1);
    }

  g_static_mutex_unlock(&self->super.lock);
}

gboolean
log_queue_disk_save_queue(LogQueue *s, gboolean *persistent)
{
  LogQueueDisk *self = (LogQueueDisk *) s;

  if (!qdisk_initialized(self->qdisk))
    {
      *persistent = FALSE;
      return TRUE;
    }

  if (self->save_queue)
    return self->save_queue(self, persistent);
  return FALSE;
}

gboolean
log_queue_disk_load_queue(LogQueue *s, const gchar *filename)
{
  LogQueueDisk *self = (LogQueueDisk *) s;

  /* qdisk portion is not yet started when this happens */
  g_assert(!qdisk_initialized(self->qdisk));

  if (self->load_queue)
    return self->load_queue(self, filename);
  return FALSE;
}

gboolean
log_queue_disk_is_reliable(LogQueue *s)
{
  LogQueueDisk *self = (LogQueueDisk *) s;
  if (self->is_reliable)
    return self->is_reliable(self);
  return FALSE;
}

const gchar *
log_queue_disk_get_filename(LogQueue *s)
{
  LogQueueDisk *self = (LogQueueDisk *) s;
  return qdisk_get_filename(self->qdisk);
}

static void
log_queue_disk_free(LogQueue *s)
{
  LogQueueDisk *self = (LogQueueDisk *) s;

  if (self->free_fn)
    self->free_fn(self);

  qdisk_deinit(self->qdisk);
  qdisk_free(self->qdisk);
  g_free(self);
}

gboolean
__pop_disk(LogQueueDisk *self, LogMessage **msg)
{
  GString *serialized;
  SerializeArchive *sa;

  *msg = NULL;

  if (!qdisk_initialized(self->qdisk))
    return FALSE;

  serialized = g_string_sized_new(64);
  if (!qdisk_pop_head(self->qdisk, serialized))
    {
      g_string_free(serialized, TRUE);
      return FALSE;
    }

  sa = serialize_string_archive_new(serialized);
  *msg = log_msg_new_empty();

  if (!log_msg_deserialize(qdisk_get_serializer(self->qdisk), *msg, sa))
  {
      g_string_free(serialized, TRUE);
      serialize_archive_free(sa);
      log_msg_unref(*msg);
      *msg = NULL;
      msg_error("Can't read correct message from disk-queue file",evt_tag_str("filename",qdisk_get_filename(self->qdisk)),NULL);
      return TRUE;
  }

  serialize_archive_free(sa);

  g_string_free(serialized, TRUE);
  return TRUE;
}

LogMessage *
__read_message(LogQueueDisk *self, LogPathOptions *path_options)
{
  LogMessage *msg = NULL;
  do
    {
      if (qdisk_get_length (self->qdisk) == 0)
        {
          break;
        }
      if (!__pop_disk (self, &msg))
        {
          msg_error("Error reading from disk-queue file, dropping disk queue",
                    evt_tag_str ("filename", qdisk_get_filename (self->qdisk)), NULL);
          self->restart_corrupted(self);
          if (msg)
            log_msg_unref (msg);
          msg = NULL;
          return NULL;
        }
    }
  while (msg == NULL);
  return msg;
}

gboolean
__write_message(LogQueueDisk *self, LogMessage *msg)
{
  GString *serialized;
  SerializeArchive *sa;
  gboolean consumed = FALSE;
  if (qdisk_initialized(self->qdisk) && qdisk_is_space_avail(self->qdisk, 64))
    {
      serialized = g_string_sized_new(64);
      sa = serialize_string_archive_new(serialized);
      log_msg_serialize(qdisk_get_serializer(self->qdisk), msg, sa);
      consumed = qdisk_push_tail(self->qdisk, serialized);
      serialize_archive_free(sa);
      g_string_free(serialized, TRUE);
    }
  return consumed;
}

static void
__restart_diskq(LogQueueDisk *self, gboolean corrupted)
{
  gchar *filename = g_strdup(qdisk_get_filename(self->qdisk));
  gchar *new_file = NULL;
  qdisk_deinit(self->qdisk);
  if (corrupted)
    {
      new_file = g_strdup_printf("%s.corrupted",filename);
      rename(filename,new_file);
      g_free(new_file);
    }
  if (self->start)
    {
      self->start(self, filename);
    }
  g_free(filename);
}

void
__restart(LogQueueDisk *self)
{
  __restart_diskq(self, FALSE);
}

void
__restart_corrupted(LogQueueDisk *self)
{
  __restart_diskq(self, TRUE);
}


void
log_queue_disk_init_instance(LogQueueDisk *self)
{
  log_queue_init_instance(&self->super,NULL);
  self->qdisk = qdisk_new();

  self->super.get_length = log_queue_disk_get_length;
  self->super.push_tail = log_queue_disk_push_tail;
  self->super.push_head = log_queue_disk_push_head;
  self->super.pop_head = log_queue_disk_pop_head;
  self->super.ack_backlog = log_queue_disk_ack_backlog;
  self->super.rewind_backlog = log_queue_disk_rewind_backlog;
  self->super.rewind_backlog_all = log_queue_disk_rewind_backlog_all;
  self->super.free_fn = log_queue_disk_free;

  self->read_message = __read_message;
  self->write_message = __write_message;
  self->restart = __restart;
  self->restart_corrupted = __restart_corrupted;
}
