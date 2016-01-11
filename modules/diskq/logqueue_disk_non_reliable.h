/*
 * log_queue_disk_non_reliable.h
 *
 *  Created on: Sep 14, 2014
 *      Author: jviktor
 */

#ifndef LOG_QUEUE_DISK_NON_RELIABLE_H_
#define LOG_QUEUE_DISK_NON_RELIABLE_H_

#include "logqueue_disk.h"

LogQueue *log_queue_disk_non_reliable_new(gint64 qdisk_size, gint qout_size, gint qoverflow_size, LogMsgSerializer *serializer);
LogQueue *log_queue_disk_non_reliable_read_only_new(gint64 qdisk_size, gint qout_size, gint qoverflow_size, LogMsgSerializer *serializer);


#endif /* LOG_QUEUE_DISK_NON_RELIABLE_H_ */
