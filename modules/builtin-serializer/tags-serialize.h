#ifndef TAGS_SERIALIZE_H
#define TAGS_SERIALIZE_H

#include "logmsg.h"
#include "serialize.h"

gboolean tags_deserialize(LogMessage *msg, SerializeArchive *sa);
gboolean tags_serialize(LogMessage *msg, SerializeArchive *sa);

#endif
