/*
 * logqueue_disk_reliable.h
 *
 *  Created on: Sep 14, 2014
 *      Author: jviktor
 */

#ifndef LOGQUEUE_DISK_RELIABLE_H_
#define LOGQUEUE_DISK_RELIABLE_H_

#include "logqueue_disk.h"

LogQueue *log_queue_disk_reliable_new(gint64 qdisk_size, gint msg_buf_size, LogMsgSerializer *serializer);
LogQueue *log_queue_disk_reliable_read_only_new(gint64 qdisk_size, gint msg_buf_size, LogMsgSerializer *serializer);

#endif /* LOGQUEUE_DISK_RELIABLE_H_ */
