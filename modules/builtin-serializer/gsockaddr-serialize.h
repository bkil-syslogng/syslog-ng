#ifndef G_SOCKADDR_SERIALIZE_H
#define G_SOCKADDR_SERIALIZE_H

#include "gsockaddr.h"
#include "serialize.h"

gboolean g_sockaddr_serialize(SerializeArchive *sa, GSockAddr *addr);
gboolean g_sockaddr_deserialize(SerializeArchive *sa, GSockAddr **addr);

#endif
