/*
 * Copyright (C) 2006-2011 BalaBit IT Ltd.
 *
 * All rights reserved.
 */
#ifndef DISKQ_H
#define DISKQ_H

#include "config.h"
#include "logqueue.h"
#include "driver.h"
#include "logmsg-serializer.h"

typedef struct _DiskQDestPlugin
{
  LogDriverPlugin super;
  gint mem_buf_length;
  gint64 disk_buf_size;
  gboolean reliable;
  gint mem_buf_size;
  gint qout_size;
  LogMsgSerializer *serializer;
} DiskQDestPlugin;

DiskQDestPlugin *diskq_dest_plugin_new(void);

void diskq_log_qout_size_set(DiskQDestPlugin *self, gint qout_size);
void diskq_disk_buf_size_set(DiskQDestPlugin *self, gint64 disk_buf_size);
void diskq_reliable_set(DiskQDestPlugin *self, gboolean reliable);
void diskq_mem_buf_size_set(DiskQDestPlugin *self, gint mem_buf_size);
void diskq_mem_buf_length_set(DiskQDestPlugin *self, gint mem_buf_length);
void diskq_check_plugin_settings(DiskQDestPlugin *self);
void diskq_set_serializer(DiskQDestPlugin *self, LogMsgSerializer *serializer);

#endif
