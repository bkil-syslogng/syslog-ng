/*
 * Copyright (C) 2006-2011 BalaBit IT Ltd.
 *
 * All rights reserved.
 */
#include "driver.h"
#include "messages.h"

#include "diskq.h"
#include "logqueue_disk.h"
#include "logqueue_disk_reliable.h"
#include "logqueue_disk_non_reliable.h"
#include "persist-state.h"

#define MIN_DISK_BUF_SIZE 1024*1024

void
diskq_log_qout_size_set(DiskQDestPlugin *self, gint qout_size)
{
  if (qout_size < 64)
    {
      msg_warning("WARNING: The configured qout size is smaller than the minimum allowed",
                  evt_tag_int("configured size", qout_size),
                  evt_tag_int("minimum allowed size", 64),
                  evt_tag_int("new size", 64),
                  NULL);
      qout_size = 64;
    }
  self->options.qout_size = qout_size;
}

void
diskq_disk_buf_size_set(DiskQDestPlugin *self, gint64 disk_buf_size)
{
  if (disk_buf_size == 0)
    {
      msg_warning("WARNING: The configured disk buffer size is zero. No disk queue file will be created.", NULL);
    }
  else if (disk_buf_size < MIN_DISK_BUF_SIZE)
    {
      msg_warning("WARNING: The configured disk buffer size is smaller than the minimum allowed",
                  evt_tag_int("configured size", disk_buf_size),
                  evt_tag_int("minimum allowed size", MIN_DISK_BUF_SIZE),
                  evt_tag_int("new size", MIN_DISK_BUF_SIZE),
                  NULL);
      disk_buf_size = MIN_DISK_BUF_SIZE;
    }
  self->options.disk_buf_size = disk_buf_size;
}

void
diskq_reliable_set(DiskQDestPlugin *self, gboolean reliable)
{
  self->options.reliable = reliable;
}

void
diskq_mem_buf_size_set(DiskQDestPlugin *self, gint mem_buf_size)
{
  self->options.mem_buf_size = mem_buf_size;
}

void
diskq_mem_buf_length_set(DiskQDestPlugin *self, gint mem_buf_length)
{
  self->options.mem_buf_length = mem_buf_length;
}

void
diskq_check_plugin_settings(DiskQDestPlugin *self)
{
  if (self->options.reliable)
    {
      if (self->options.mem_buf_length > 0)
        {
          msg_warning("WARNING: Reliable queue: the mem-buf-length parameter is omitted", NULL);
        }
    }
  else
    {
      if (self->options.mem_buf_size > 0)
        {
          msg_warning("WARNING: Non-reliable queue: the mem-buf-size parameter is omitted", NULL);
        }
    }
}

void
diskq_set_serializer(DiskQDestPlugin *self, LogMsgSerializer *serializer)
{
  self->options.serializer = serializer;
}

/*
 * NOTE: we don't invoke the inherited acquire_queue() functionality,
 * so if there are multiple plugins registered to this same hook, this
 * will completely hide it away.
 */
LogQueue *
diskq_dest_plugin_acquire_queue(LogDestDriver *dd, gchar *persist_name, gpointer user_data)
{
  DiskQDestPlugin *self = (DiskQDestPlugin *) user_data;
  GlobalConfig *cfg = log_pipe_get_config(&dd->super.super);
  LogQueue *queue = NULL;
  gchar *qfile_name;
  GError *error = NULL;
  gboolean success;

  if (persist_name)
    queue = cfg_persist_config_fetch(cfg, persist_name);

  if (queue)
    {
      if (queue->type != log_queue_disk_type || self->options.reliable != log_queue_disk_is_reliable(queue))
        {
          log_queue_unref(queue);
          queue = NULL;
        }
    }

  if (!self->options.serializer)
    {
      self->options.serializer = log_msg_serializer_factory(cfg, "builtin", &error);
    }

  if (!self->options.serializer)
    {
      msg_error("Can't load the builtin serializer plugin",
                    evt_tag_str("error", error->message),
                    NULL);
      return NULL;
    }

  if (!queue)
    {
      if (self->options.reliable)
        queue = log_queue_disk_reliable_new(&self->options);
      else
        queue = log_queue_disk_non_reliable_new(&self->options);
      log_queue_set_throttle(queue, dd->throttle);
      queue->persist_name = g_strdup(persist_name);
    }

  qfile_name = persist_state_lookup_string(cfg->state, persist_name, NULL, NULL);
  success = log_queue_disk_load_queue(queue, qfile_name);
  if (!success)
    {
      if (qfile_name && log_queue_disk_load_queue(queue, NULL))
        {
          msg_error("Error opening disk-queue file, a new one started",
                    evt_tag_str("old_filename", qfile_name),
                    evt_tag_str("new_filename", log_queue_disk_get_filename(queue)),
                    NULL);
          g_free(qfile_name);
        }
      else
        {
          g_free(qfile_name);
          msg_error("Error initializing log queue", NULL);
          return NULL;
        }
    }

  if (persist_name)
    {
      /* save the queue file name to permanent state */
      qfile_name = (gchar *) log_queue_disk_get_filename(queue);
      if (qfile_name)
        {
          persist_state_alloc_string(cfg->state, persist_name, qfile_name, -1);
        }
    }

  return queue;
}

void
diskq_dest_plugin_release_queue(LogDestDriver *dd, LogQueue *queue, gpointer user_data)
{
  GlobalConfig *cfg = log_pipe_get_config(&dd->super.super);
  gboolean persistent;

  log_queue_disk_save_queue(queue, &persistent);
  if (queue->persist_name)
    {
      cfg_persist_config_add(cfg, queue->persist_name, queue, (GDestroyNotify) log_queue_unref, FALSE);
    }
  else
    {
      log_queue_unref(queue);
    }
}

static gboolean
diskq_dest_plugin_attach(LogDriverPlugin *s, LogDriver *d)
{
  DiskQDestPlugin *self = (DiskQDestPlugin *) s;
  LogDestDriver *dd = (LogDestDriver *) d;
  GlobalConfig *cfg = log_pipe_get_config(&d->super);

  if (self->options.disk_buf_size == -1)
    {
      msg_error("The required 'disk_buf_size()' parameter of diskq module has not been set.", NULL);
      return FALSE;
    }

  if (self->options.disk_buf_size < MIN_DISK_BUF_SIZE && self->options.disk_buf_size != 0)
    {
      msg_warning("The value of 'disk_buf_size()' is too low, setting to the smallest acceptable value",
                  evt_tag_int("min_space", MIN_DISK_BUF_SIZE),
                  NULL);
      self->options.disk_buf_size = MIN_DISK_BUF_SIZE;
    }

  if (dd->acquire_queue_data || dd->release_queue_data)
    if (dd->acquire_queue_data != self)
      {
        msg_error("Another queueing plugin is registered in this destination, unable to register diskq again",
                  evt_tag_str("driver", dd->super.id),
                  NULL);
        return FALSE;
      }

  if (self->options.mem_buf_length < 0)
    self->options.mem_buf_length = dd->log_fifo_size;
  if (self->options.mem_buf_length < 0)
    self->options.mem_buf_length = cfg->log_fifo_size;
  if (self->options.qout_size < 0)
    self->options.qout_size = 64;

  dd->acquire_queue_data = self;
  dd->acquire_queue = diskq_dest_plugin_acquire_queue;
  dd->release_queue_data = self;
  dd->release_queue = diskq_dest_plugin_release_queue;
  return TRUE;
}

void
diskq_set_dir(DiskQDestPlugin *self, const gchar *dir)
{
  if (self->options.dir)
    {
      g_free(self->options.dir);
    }
  self->options.dir = g_strdup(dir);
}

void
diskq_dest_plugin_free(LogDriverPlugin *s)
{
  DiskQDestPlugin *self = (DiskQDestPlugin *)s;
  diskq_set_dir(self, NULL);
}

static void
__set_default_options(QDiskOptions *self)
{
  self->disk_buf_size = -1;
  self->mem_buf_length = -1;
  self->reliable = FALSE;
  self->mem_buf_size = -1;
  self->qout_size = -1;
  self->dir = get_installation_path_for(PATH_LOCALSTATEDIR);
}

DiskQDestPlugin *
diskq_dest_plugin_new(void)
{
  DiskQDestPlugin *self = g_new0(DiskQDestPlugin, 1);

  log_driver_plugin_init_instance(&self->super);
  __set_default_options(&self->options);
  self->super.attach = diskq_dest_plugin_attach;
  self->super.free_fn = diskq_dest_plugin_free;
  return self;
}

