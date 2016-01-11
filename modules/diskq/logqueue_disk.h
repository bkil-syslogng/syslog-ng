/*
 * Copyright (C) 2006-2011 BalaBit IT Ltd.
 *
 * All rights reserved.
 */

#ifndef LOGQUEUE_DISK_H_INCLUDED
#define LOGQUEUE_DISK_H_INCLUDED

#include "logmsg.h"
#include "logqueue.h"
#include "qdisk.h"
#include "logmsg-serializer.h"

typedef struct _LogQueueDisk LogQueueDisk;

#define LOG_PATH_OPTIONS_FOR_BACKLOG GINT_TO_POINTER(0x80000000)

struct _LogQueueDisk
{
  LogQueue super;
  QDisk *qdisk;         /* disk based queue */
  gint64 (*get_length)(LogQueueDisk *s);
  gboolean (*push_tail)(LogQueueDisk *s, LogMessage *msg, LogPathOptions *local_options, const LogPathOptions *path_options);
  void (*push_head)(LogQueueDisk *s, LogMessage *msg, const LogPathOptions *path_options);
  LogMessage *(*pop_head)(LogQueueDisk *s, LogPathOptions *path_options);
  void (*ack_backlog)(LogQueueDisk *s, guint num_msg_to_ack);
  void (*rewind_backlog)(LogQueueDisk *s, guint rewind_count);
  gboolean (*save_queue)(LogQueueDisk *s, gboolean *persistent);
  gboolean (*load_queue)(LogQueueDisk *s, const gchar *filename);
  gboolean (*start)(LogQueueDisk *s, const gchar *filename);
  void (*free_fn)(LogQueueDisk *s);
  gboolean (*is_reliable)(LogQueueDisk *s);
  LogMessage * (*read_message)(LogQueueDisk *self, LogPathOptions *path_options);
  gboolean (*write_message)(LogQueueDisk *self, LogMessage *msg);
  void (*restart)(LogQueueDisk *self);
  void (*restart_corrupted)(LogQueueDisk *self);
};

extern const gchar *qdisk_dir;
extern const QueueType log_queue_disk_type;

gboolean log_queue_disk_is_reliable(LogQueue *s);
const gchar *log_queue_disk_get_filename(LogQueue *self);
gboolean log_queue_disk_save_queue(LogQueue *self, gboolean *persistent);
gboolean log_queue_disk_load_queue(LogQueue *self, const gchar *filename);
void log_queue_disk_init_instance(LogQueueDisk *self);

#endif
