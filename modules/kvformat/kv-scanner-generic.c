/*
 * Copyright (c) 2016 Balabit
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
#include "kv-scanner-generic.h"

typedef struct _KVToken KVToken;
struct _KVToken
{
  const gchar *begin;
  const gchar *end;
};

typedef struct _KVScannerGeneric KVScannerGeneric;
struct _KVScannerGeneric {
  KVScanner super;
  gint state;
  KVToken next_key;
  KVToken next_value;
};

enum
{
  KV_FIND_VALUE_INIT = 0,
  KV_FIND_VALUE_SEPARATOR_AFTER_INIT,
  KV_FIND_VALUE_VALUE,
  KV_FIND_VALUE_KEY_OR_VALUE,
  KV_FIND_VALUE_IN_QUOTE,
  KV_FIND_VALUE_AFTER_QUOTE,
  KV_FIND_VALUE_IN_SEPARATOR,
  KV_FIND_VALUE_FINISH,
  KV_FIND_EOL
};

static inline const gchar*
_get_current_position(KVScannerGeneric *self)
{
  return self->super.input + self->super.input_pos;
}

static void
_reset_value(KVScannerGeneric *self)
{
  g_string_truncate(self->super.value, 0);
  self->next_key.begin = NULL;
  self->next_key.end = NULL;
  self->next_value.begin = _get_current_position(self);
  self->next_value.end = _get_current_position(self);
  self->super.value_was_quoted = FALSE;
}

/*
 * Will be different from the original in the future
 */
static inline gboolean
_is_valid_key_character(char c)
{
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') ||
         (c == '_') ||
         (c == '-');
}

static inline void
_dismiss_next_key(KVScannerGeneric *self)
{
  if (self->next_key.begin < self->next_key.end)
    {
      self->next_value.end = self->next_key.end;
      self->next_key.begin = self->next_key.end = NULL;
    }
}

static inline void
_start_next_key(KVScannerGeneric *self)
{
  self->next_key.begin = self->next_key.end = _get_current_position(self);
}

static inline void
_end_next_key(KVScannerGeneric *self)
{
  self->next_key.end = _get_current_position(self);
}

static inline void
_start_value(KVScannerGeneric *self)
{
  self->next_value.begin = self->next_value.end = _get_current_position(self);
}

static inline void
_end_value(KVScannerGeneric *self)
{
  self->next_value.end = _get_current_position(self);
}

static inline void
_value_init_state(KVScannerGeneric *self)
{
  gchar ch = self->super.input[self->super.input_pos];

  if (ch == '\0')
    {
      self->state = KV_FIND_EOL;
    }
  else if (ch == ' ')
    {
      self->state = KV_FIND_VALUE_SEPARATOR_AFTER_INIT;
    }
  else if (ch == '\'' || ch == '\"')
    {
      _start_value(self);
      self->super.quote_char = ch;
      self->state = KV_FIND_VALUE_IN_QUOTE;
    }
  else
    {
      _start_value(self);
      self->state = KV_FIND_VALUE_VALUE;
    }
}

static inline void
_value_separator_after_init_state(KVScannerGeneric *self)
{
  gchar ch = self->super.input[self->super.input_pos];

  if (ch == '\0')
    {
      self->state = KV_FIND_EOL;
    }
  else if (ch == ' ')
    {
      ;
    }
  else if (ch == '\'' || ch == '\"')
    {
      _start_value(self);
      self->super.quote_char = ch;
      self->state = KV_FIND_VALUE_IN_QUOTE;
    }
  else if (_is_valid_key_character(ch))
    {
      _start_value(self);
      _start_next_key(self);
      self->state = KV_FIND_VALUE_KEY_OR_VALUE;
    }
  else
    {
      _start_value(self);
      self->state = KV_FIND_VALUE_VALUE;
    }
}

static inline void
_value_value_state(KVScannerGeneric *self)
{
  gchar ch = self->super.input[self->super.input_pos];

  if (ch == '\0')
    {
      _end_value(self);
      self->state = KV_FIND_EOL;
    }
  else if (ch == ' ')
    {
      _end_value(self);
      self->state = KV_FIND_VALUE_IN_SEPARATOR;
    }
}

static inline void
_value_key_or_value_state(KVScannerGeneric *self)
{
  gchar ch = self->super.input[self->super.input_pos];

  if (ch == '\0')
    {
      _end_next_key(self);
      _dismiss_next_key(self);
      self->state = KV_FIND_EOL;
    }
  else if (ch == ' ')
    {
      _end_next_key(self);
      self->state = KV_FIND_VALUE_IN_SEPARATOR;
    }
  else if (ch == self->super.value_separator)
    {
      _end_next_key(self);
      self->state = KV_FIND_VALUE_FINISH;
    }
  else if (_is_valid_key_character(ch))
    {
      ;
    }
  else
    {
      _dismiss_next_key(self);
      self->state = KV_FIND_VALUE_VALUE;
    }
}

static inline void
_value_in_quote_state(KVScannerGeneric *self)
{
  gchar ch = self->super.input[self->super.input_pos];

  if (ch == '\0')
    {
      _end_value(self);
      self->state = KV_FIND_EOL;
    }
  else if (ch == self->super.quote_char)
    {
      _end_value(self);
      self->state = KV_FIND_VALUE_AFTER_QUOTE;
    }
}

static inline void
_value_after_quote_state(KVScannerGeneric *self)
{
  gchar ch = self->super.input[self->super.input_pos];

  _end_value(self);
  if (ch == '\0')
    {
      self->state = KV_FIND_EOL;
    }
  else if (ch == ' ')
    {
      self->state = KV_FIND_VALUE_IN_SEPARATOR;
    }
  else if (ch == '\'' || ch == '\"')
    {
      self->super.quote_char = ch;
      self->state = KV_FIND_VALUE_IN_QUOTE;
    }
  else if (_is_valid_key_character(ch))
    {
      _start_next_key(self);
      self->state = KV_FIND_VALUE_KEY_OR_VALUE;
    }
  else
    {
      self->state = KV_FIND_VALUE_VALUE;
    }
}

static inline void
_value_in_separator_state(KVScannerGeneric *self)
{
  gchar ch = self->super.input[self->super.input_pos];

  if (ch == '\0')
    {
      _dismiss_next_key(self);
      self->state = KV_FIND_EOL;
    }
  else if (ch == ' ')
    {
      ;
    }
  else if (ch == self->super.value_separator)
    {
      self->state = KV_FIND_VALUE_FINISH;
    }
  else if (_is_valid_key_character(ch))
    {
      _dismiss_next_key(self);
      _start_next_key(self);
      self->state = KV_FIND_VALUE_KEY_OR_VALUE;
    }
  else
    {
      _dismiss_next_key(self);
      self->state = KV_FIND_VALUE_VALUE;
    }
}

static inline gboolean
_extract_value(KVScannerGeneric *self)
{
  const gchar *cur;

  _reset_value(self);
  self->state = KV_FIND_VALUE_INIT;

  while (self->state != KV_FIND_VALUE_FINISH &&
         self->state != KV_FIND_EOL)
    {
      switch (self->state)
        {
        case KV_FIND_VALUE_INIT:
          _value_init_state(self);
          break;
        case KV_FIND_VALUE_SEPARATOR_AFTER_INIT:
          _value_separator_after_init_state(self);
          break;
        case KV_FIND_VALUE_VALUE:
          _value_value_state(self);
          break;
        case KV_FIND_VALUE_KEY_OR_VALUE:
          _value_key_or_value_state(self);
          break;
        case KV_FIND_VALUE_IN_QUOTE:
          _value_in_quote_state(self);
          break;
        case KV_FIND_VALUE_AFTER_QUOTE:
          _value_after_quote_state(self);
          break;
        case KV_FIND_VALUE_IN_SEPARATOR:
          _value_in_separator_state(self);
          break;
        }
      self->super.input_pos++;
    }

  if (*self->next_value.begin == self->super.quote_char && *(self->next_value.end-1) == self->super.quote_char)
    {
      self->super.value_was_quoted = TRUE;
      self->next_value.begin++;
      self->next_value.end--;
    }

  for (cur = self->next_value.begin; cur < self->next_value.end ; cur++)
    {
      g_string_append_c(self->super.value, *cur);
    }

  return TRUE;
}

static inline gboolean
_find_first_key(KVScannerGeneric *self)
{
  const gchar *separator, *start_of_key, *end_of_key;
  gint len;

  separator = strchr(self->super.input, self->super.value_separator);

  do
    {
      if (!separator)
        return FALSE;

      end_of_key = separator-1;
      while (end_of_key >= self->super.input && *end_of_key == ' ')
        end_of_key--;

      start_of_key = end_of_key;

      while (start_of_key >= self->super.input && _is_valid_key_character(*start_of_key))
        start_of_key--;

      len = end_of_key - start_of_key;

      if (len < 1)
        separator = strchr(separator + 1, self->super.value_separator);
    }
  while (len < 1);

  self->next_key.begin = start_of_key + 1;
  self->next_key.end = end_of_key + 1;
  self->super.input_pos = separator - self->super.input + 1;
  return TRUE;
}

static inline gboolean
_extract_key(KVScannerGeneric *self)
{

  if (self->state == KV_FIND_EOL || self->super.input[self->super.input_pos] == '\0')
    {
      return FALSE;
    }

  if ((self->next_key.begin == NULL || self->next_key.end == NULL) && !_find_first_key(self))
    {
      return FALSE;
    }

  g_string_assign_len(self->super.key, self->next_key.begin, self->next_key.end - self->next_key.begin);

  return TRUE;
}

static gboolean
_scan_next(KVScanner* s)
{
  KVScannerGeneric* self = (KVScannerGeneric*)s;

  return _extract_key(self) && _extract_value(self) && kv_scanner_decode_value(s);
}

KVScanner* kv_scanner_generic_new(gchar value_separator)
{
 KVScannerGeneric *self = g_new0(KVScannerGeneric, 1);

 kv_scanner_init(&self->super, value_separator);
 self->super.scan_next = _scan_next;

 return &self->super;
}
