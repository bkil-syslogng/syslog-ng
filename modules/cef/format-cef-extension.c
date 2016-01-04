/*
 * Copyright (c) 2015 Balabit
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
 */

#include "filter/filter-expr.h"
#include "filter/filter-expr-parser.h"
#include "cfg.h"
#include "value-pairs.h"
#include "vptransform.h"
#include "syslog-ng.h"
#include "utf8utils.h"
#include "format-cef-extension.h"

typedef struct _TFCefState
{
  TFSimpleFuncState super;
  ValuePairs *vp;
} TFCefState;

static gboolean
tf_cef_prepare(LogTemplateFunction *self, gpointer s, LogTemplate *parent,
		gint argc, gchar *argv[],
		GError **error)
{
  TFCefState *state = (TFCefState *)s;

  state->vp = value_pairs_new_from_cmdline (parent->cfg, argc, argv, error);
  if (!state->vp)
    return FALSE;

  return TRUE;
}

typedef struct
{
  gboolean need_separator;
  GString *buffer;
  const LogTemplateOptions *template_options;
} CefWalkerState;

static gboolean
tf_cef_is_valid_key (const gchar *str)
{
  size_t end = strspn (str, "0123456789"
                       "abcdefghijklmnopqrstuvwxyz"
                       "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  return str[end] == '\0';
}

static inline void
tf_cef_append_escaped (GString *escaped_string, const gchar *str)
{
  const gchar *char_ptr = str;
  const gchar *const end = char_ptr + strlen(str);

  while (char_ptr < end)
    {
      gunichar uchar = g_utf8_get_char_validated (char_ptr, end - char_ptr);

      if (!g_unichar_isdefined(uchar))
        {
          g_string_append_printf (escaped_string, "\\x%02x",
                                  *(guint8 *) char_ptr);
          char_ptr++;
        }
      else
        {
          switch (uchar)
            {
            case '=':
              g_string_append (escaped_string, "\\=");
              break;
            case '\n':
              g_string_append (escaped_string, "\\n");
              break;
            case '\r':
              g_string_append (escaped_string, "\\r");
              break;
            case '\\':
              g_string_append (escaped_string, "\\\\");
              break;
            default:
              if (uchar < 32)
                g_string_append_printf (escaped_string, "\\u%04x", uchar);
              else
                g_string_append_unichar (escaped_string, uchar);
              break;
            }
          char_ptr = g_utf8_next_char (char_ptr);
        }
    }
}

static gboolean
tf_cef_append_value(const gchar *name, const gchar *value,
                     CefWalkerState *state)
{
  if (state->need_separator)
    g_string_append_c(state->buffer, ' ');

  g_string_append(state->buffer, name);

  g_string_append_c(state->buffer, '=');

  tf_cef_append_escaped(state->buffer, value);

  return TRUE;
}

static gint
tf_cef_walk_cmp(const gchar *s1, const gchar *s2)
{
  return strcmp(s1, s2);
}

static gboolean
tf_cef_walker(const gchar *name, TypeHint type, const gchar *value,
              gpointer user_data)
{
  CefWalkerState *state = (CefWalkerState *)user_data;
  gint on_error = state->template_options->on_error;

  if (!tf_cef_is_valid_key(name))
    return !!(on_error & ON_ERROR_DROP_MESSAGE);

  tf_cef_append_value(name, value, state);

  state->need_separator = TRUE;

  return FALSE;
}

static gboolean
tf_cef_append(GString *result, ValuePairs *vp, LogMessage *msg,
               const LogTemplateOptions *template_options, gint32 seq_num, gint time_zone_mode)
{
  CefWalkerState state;

  state.need_separator = FALSE;
  state.buffer = result;
  state.template_options = template_options;

  return value_pairs_foreach_sorted (vp, tf_cef_walker,
                                     (GCompareDataFunc) tf_cef_walk_cmp, msg,
                                     seq_num, time_zone_mode, template_options,
                                     &state);
}

static void
tf_cef_call(LogTemplateFunction *self, gpointer s,
	     const LogTemplateInvokeArgs *args, GString *result)
{
  TFCefState *state = (TFCefState *)s;
  gint i;
  gboolean r = TRUE;
  gsize orig_size = result->len;

  for (i = 0; i < args->num_messages; i++)
    r &= tf_cef_append(result, state->vp, args->messages[i], args->opts, args->seq_num, args->tz);

  if (!r && (args->opts->on_error & ON_ERROR_DROP_MESSAGE))
    g_string_set_size(result, orig_size);
}

static void
tf_cef_free_state(gpointer s)
{
  TFCefState *state = (TFCefState *)s;

  if (state->vp)
    value_pairs_unref(state->vp);
  tf_simple_func_free_state(&state->super);
}

TEMPLATE_FUNCTION(TFCefState, tf_cef, tf_cef_prepare, NULL, tf_cef_call,
		  tf_cef_free_state, NULL);
