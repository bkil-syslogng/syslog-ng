#include "gsockaddr-serialize.h"

static void
__serialize_ipv4(GSockAddr* addr, SerializeArchive* sa)
{
  struct in_addr ina;
  ina = g_sockaddr_inet_get_address(addr);
  serialize_write_blob(sa, (gchar*) &ina, sizeof(ina));
  serialize_write_uint16(sa, htons(g_sockaddr_get_port(addr)));
}

static void
__serialize_ipv6(GSockAddr* addr, SerializeArchive* sa)
{
#if SYSLOG_NG_ENABLE_IPV6
  struct in6_addr* in6a;
  in6a = g_sockaddr_inet6_get_address(addr);
  serialize_write_blob(sa, (gchar*) in6a, sizeof(*in6a));
  serialize_write_uint16(sa, htons(g_sockaddr_get_port(addr)));
#endif
}

gboolean
g_sockaddr_serialize(SerializeArchive *sa, GSockAddr *addr)
{
  if (!addr)
    {
      serialize_write_uint16(sa, 0);
      return TRUE;
    }

  serialize_write_uint16(sa, addr->sa.sa_family);
  switch (addr->sa.sa_family)
    {
    case AF_INET:
      {
        __serialize_ipv4(addr, sa);
        break;
      }
#if SYSLOG_NG_ENABLE_IPV6
    case AF_INET6:
      {
        __serialize_ipv6(addr, sa);
        break;
      }
#endif
    default:
      break;
    }
  return TRUE;
}

static gboolean
__deserialize_ipv4(SerializeArchive *sa, GSockAddr **addr)
{
  struct sockaddr_in sin;

  sin.sin_family = AF_INET;
  if (!serialize_read_blob(sa, (gchar *) &sin.sin_addr, sizeof(sin.sin_addr)) ||
      !serialize_read_uint16(sa, &sin.sin_port))
    return FALSE;

  *addr = g_sockaddr_inet_new2(&sin);
  return TRUE;
}

static gboolean
__deserialize_ipv6(SerializeArchive *sa, GSockAddr **addr)
{
  gboolean result = FALSE;
#if SYSLOG_NG_ENABLE_IPV6
  struct sockaddr_in6 sin6;

  sin6.sin6_family = AF_INET6;
  if (serialize_read_blob(sa, (gchar *) &sin6.sin6_addr, sizeof(sin6.sin6_addr)) &&
      serialize_read_uint16(sa, &sin6.sin6_port))
    {
      *addr = g_sockaddr_inet6_new2(&sin6);
      result = TRUE;
    }
#endif
  return result;
}

gboolean
g_sockaddr_deserialize(SerializeArchive *sa, GSockAddr **addr)
{
  guint16 family;
  gboolean result = TRUE;

  if (!serialize_read_uint16(sa, &family))
    return FALSE;

  switch (family)
    {
    case 0:
      /* special case, no address were stored */
      *addr = NULL;
      break;
    case AF_INET:
      {
        result = __deserialize_ipv4(sa, addr);
        break;
      }
#if SYSLOG_NG_ENABLE_IPV6
    case AF_INET6:
      {
        result = __deserialize_ipv6(sa, addr);
        break;
      }
#endif
    case AF_UNIX:
      *addr = g_sockaddr_unix_new(NULL);
      break;
    default:
      result = FALSE;
      break;
    }
  return result;
}


