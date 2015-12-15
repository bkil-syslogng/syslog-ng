#ifndef SDATA_SERIALIZE_H
#define SDATA_SERIALIZE_H

#include "logmsg.h"
#include "serialize.h"

gboolean sdata_deserialize(LogMessage *msg, SerializeArchive *sa);

#endif
