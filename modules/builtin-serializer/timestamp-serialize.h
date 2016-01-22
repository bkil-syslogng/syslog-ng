#ifndef TIMESTAMP_SERIALIZE_H
#define TIMESTAMP_SERIALIZE_H

#include "logmsg.h"
#include "serialize.h"

gboolean timestamp_serialize(SerializeArchive *sa, LogStamp *timestamps);

gboolean timestamp_deserialize(SerializeArchive *sa, LogStamp *timestamps);


#endif
