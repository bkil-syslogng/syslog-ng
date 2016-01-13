/*
 * qdisk.h
 *
 *  Created on: Sep 13, 2014
 *      Author: jviktor
 */

#ifndef QDISK_H_
#define QDISK_H_

#include "syslog-ng.h"
#include "logmsg-serializer.h"
#include "diskq.h"

#define QDISK_RESERVED_SPACE 4096
#define LOG_PATH_OPTIONS_TO_POINTER(lpo) GUINT_TO_POINTER(0x80000000 | (lpo)->ack_needed)

/* NOTE: this must not evaluate ptr multiple times, otherwise the code that
 * uses this breaks, as it passes the result of a g_queue_pop_head call,
 * which has side effects.
 */
#define POINTER_TO_LOG_PATH_OPTIONS(ptr, lpo) (lpo)->ack_needed = (GPOINTER_TO_INT(ptr) & ~0x80000000)

typedef struct _QDisk QDisk;

QDisk *qdisk_new();

gboolean qdisk_is_space_avail(QDisk *self, gint at_least);
gboolean qdisk_push_tail(QDisk *self, GString *record);
gboolean qdisk_pop_head(QDisk *self, GString *record);
gboolean qdisk_start(QDisk *self, const gchar *filename, GQueue *qout, GQueue *qbacklog, GQueue *qoverflow);
void qdisk_init(QDisk *self, QDiskOptions *options);
void qdisk_deinit(QDisk *self);
void qdisk_reset_file_if_possible(QDisk *self);
gboolean qdisk_initialized(QDisk *self);
void qdisk_free(QDisk *self);

gboolean qdisk_save_state(QDisk *self, GQueue *qout, GQueue *qbacklog, GQueue *qoverflow);

gint64 qdisk_get_length(QDisk *self);
void qdisk_set_length(QDisk *self, gint64 new_value);
gint64 qdisk_get_size(QDisk *self);
gint64 qdisk_get_writer_head(QDisk *self);
gint64 qdisk_get_reader_head(QDisk *self);
void qdisk_set_reader_head(QDisk *self, gint64 new_value);
gint64 qdisk_get_backlog_head(QDisk *self);
void qdisk_set_backlog_head(QDisk *self, gint64 new_value);
void qdisk_inc_backlog(QDisk *self);
void qdisk_dec_backlog(QDisk *self);
gint64 qdisk_get_backlog_count(QDisk *self);
void qdisk_set_backlog_count(QDisk *self, gint64 new_value);
gint qdisk_get_memory_size(QDisk *self);
gboolean qdisk_is_read_only(QDisk *self);
const gchar *qdisk_get_filename(QDisk *self);

gssize qdisk_read_from_backlog(QDisk *self, gpointer buffer, gsize bytes_to_read);
gssize qdisk_read(QDisk *self, gpointer buffer, gsize bytes_to_read, gint64 position);
guint64 qdisk_skip_record(QDisk *self, guint64 position);
LogMsgSerializer *qdisk_get_serializer(QDisk *self);
#endif /* QDISK_H_ */
