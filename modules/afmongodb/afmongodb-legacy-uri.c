/*
 * Copyright (c) 2010-2016 Balabit
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */

#include "afmongodb-legacy-uri.h"
#include "afmongodb-legacy-private.h"

#define DEFAULTHOST "127.0.0.1"
#define SOCKET_TIMEOUT_FOR_MONGO_CONNECTION_IN_MILLISECS 60000

static gboolean
_parse_addr(const char *str, char **host, gint *port)
{
  if (!host || !port)
    {
      msg_debug("Host or port reference should not be NULL", NULL);
      return FALSE;
    }
  char *proto_str = g_strdup_printf("mongodb://%s", str);
  mongoc_uri_t *uri = mongoc_uri_new(proto_str);
  if (!uri)
    {
      msg_error("Cannot parse MongoDB URI", evt_tag_str("uri", proto_str), NULL);
      g_free(proto_str);
      return FALSE;
    }

  const mongoc_host_list_t *hosts = mongoc_uri_get_hosts(uri);
  if (!hosts || hosts->next)
    {
      if (hosts)
        msg_error("No host found in MongoDB URI", evt_tag_str("uri", proto_str), NULL);
      else
        msg_error("Multiple hosts found in MongoDB URI", evt_tag_str("uri", proto_str), NULL);
      g_free(proto_str);
      mongoc_uri_destroy(uri);
      return FALSE;
    }
  *port = hosts->port;
  *host = g_strdup(hosts->host);
  mongoc_uri_destroy(uri);
  if (!*host)
    {
      msg_error("NULL hostname", evt_tag_str("uri", proto_str), NULL);
      g_free(proto_str);
      return FALSE;
    }
  g_free(proto_str);
  return TRUE;
}

static void
_free_host_port(gpointer data)
{
  MongoDBHostPort *hp = (MongoDBHostPort *)data;
  g_free(hp->host);
  hp->host = NULL;
  g_free(hp);
}

static gboolean
_append_host(GList **list, const char *host, gint port)
{
  if (!list)
    return FALSE;
  MongoDBHostPort *hp = g_new0(MongoDBHostPort, 1);
  hp->host = g_strdup(host);
  hp->port = port;
  *list = g_list_prepend(*list, hp);
  return TRUE;
}

static gboolean
_append_legacy_servers(MongoDBDestDriver *self)
{
  if (self->port != MONGO_CONN_LOCAL)
    {
      if (self->address)
        {
          gchar *srv = g_strdup_printf("%s:%d", self->address,
                                       (self->port) ? self->port : MONGOC_DEFAULT_PORT);
          self->servers = g_list_prepend(self->servers, srv);
          g_free(self->address);
        }

      if (self->servers)
        {
          GList *l;

          for (l = self->servers; l; l = g_list_next(l))
            {
              gchar *host = NULL;
              gint port = MONGOC_DEFAULT_PORT;

              if (!_parse_addr(l->data, &host, &port))
                {
                  msg_warning("Cannot parse MongoDB server address, ignoring",
                              evt_tag_str("address", l->data),
                              evt_tag_str("driver", self->super.super.super.id),
                              NULL);
                  continue;
                }
              _append_host(&self->recovery_cache, host, port);
              msg_verbose("Added MongoDB server seed",
                          evt_tag_str("host", host),
                          evt_tag_int("port", port),
                          evt_tag_str("driver", self->super.super.super.id),
                          NULL);
              g_free(host);
            }
        }
      else
        {
          gchar *localhost = g_strdup_printf(DEFAULTHOST ":%d", self->port);
          self->servers = g_list_append(NULL, localhost);
          _append_host(&self->recovery_cache, DEFAULTHOST, self->port);
        }

      self->address = NULL;
      self->port = MONGOC_DEFAULT_PORT;
      if (!_parse_addr(g_list_nth_data(self->servers, 0), &self->address, &self->port))
        {
          msg_error("Cannot parse the primary host",
                    evt_tag_str("primary", g_list_nth_data(self->servers, 0)),
                    evt_tag_str("driver", self->super.super.super.id),
                    NULL);
          return FALSE;
        }
    }
  else
    {
      if (!self->address)
        {
          msg_error("Cannot parse address",
                    evt_tag_str("primary", g_list_nth_data(self->servers, 0)),
                    evt_tag_str("driver", self->super.super.super.id),
                    NULL);
          return FALSE;
        }
      _append_host(&self->recovery_cache, self->address, 0);
    }
  return TRUE;
}

static gboolean
_append_servers(GString *uri_str, const GList *recovery_cache, gboolean *have_uri)
{
  const GList *iterator = recovery_cache;
  *have_uri = FALSE;
  gboolean have_path = FALSE;
  do
    {
      const MongoDBHostPort *hp = (const MongoDBHostPort *)iterator->data;
      if (hp->port)
        {
          *have_uri = TRUE;
          if (have_path)
            {
              msg_warning("Cannot specify both a domain socket and address", NULL);
              return FALSE;
            }
          g_string_append_printf(uri_str, "%s:%hu", hp->host, hp->port);
        }
      else
        {
          have_path = TRUE;
          if (*have_uri)
            {
              msg_warning("Cannot specify both a domain socket and address", NULL);
              return FALSE;
            }
          g_string_append_printf(uri_str, "%s", hp->host);
        }
      iterator = iterator->next;
      if (iterator)
        g_string_append_printf(uri_str, ",");
    }
  while (iterator);
  return TRUE;
}

static gboolean
_check_auth_options(MongoDBDestDriver *self)
{
  if (self->user || self->password)
    {
      if (!self->user || !self->password)
        {
          msg_error("Neither the username, nor the password can be empty", NULL);
          return FALSE;
        }
    }
  return TRUE;
}

gboolean
afmongodb_dd_create_uri_from_legacy(MongoDBDestDriver *self)
{
  if (self->uri_str)
    msg_trace("create_uri", evt_tag_str("uri_str", self->uri_str->str), NULL);
  msg_trace("create_uri", evt_tag_int("is_legacy", self->is_legacy), NULL);

  if (!_check_auth_options(self))
    return FALSE;

  if (self->uri_str && self->is_legacy)
    {
      msg_error("Error: either specify a MongoDB URI (and optional collection) or only legacy options",
                evt_tag_str("driver", self->super.super.super.id),
                NULL);
      return FALSE;
    }
  else if (self->is_legacy)
    {
      _append_legacy_servers(self);

      self->uri_str = g_string_new("mongodb://");
      if (!self->uri_str)
        return FALSE;

      if (self->user && self->password)
        {
          g_string_append_printf(self->uri_str, "%s:%s", self->user, self->password);
        }

      if (!self->recovery_cache)
        {
          msg_error("Error in host server list", evt_tag_str("driver", self->super.super.super.id), NULL);
          return FALSE;
        }

      gboolean have_uri;
      if (!_append_servers(self->uri_str, self->recovery_cache, &have_uri))
        return FALSE;

      if (have_uri)
        g_string_append_printf(self->uri_str, "/%s", self->db);

      g_string_append_printf(self->uri_str, "?slaveOk=true&sockettimeoutms=%d",
      SOCKET_TIMEOUT_FOR_MONGO_CONNECTION_IN_MILLISECS);
    }

  return TRUE;
}

void
afmongodb_dd_init_legacy(MongoDBDestDriver *self)
{
  self->db = g_strdup("syslog");
  self->safe_mode = TRUE;
}

void
afmongodb_dd_free_legacy(MongoDBDestDriver *self)
{
  g_free(self->db);
  g_free(self->user);
  g_free(self->password);
  g_free(self->address);
  string_list_free(self->servers);
  g_list_free_full(self->recovery_cache, (GDestroyNotify)&_free_host_port);
  self->recovery_cache = NULL;
}

