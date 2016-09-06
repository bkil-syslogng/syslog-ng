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
#include "misc.h"

enum {
  KV_FIND_FIRST_KEY_TRIM = 0,
  KV_FIND_FIRST_KEY_IN_KEY,
  KV_FIND_FIRST_KEY_IN_SEPARATOR,
  KV_FIND_FIRST_KEY_FINISH,
  KV_FIND_VALUE_INIT,
  KV_FIND_VALUE_SEPARATOR_AFTER_INIT,
  KV_FIND_VALUE_VALUE,
  KV_FIND_VALUE_KEY_OR_VALUE,
  KV_FIND_VALUE_IN_QUOTE,
  KV_FIND_VALUE_AFTER_QUOTE,
  KV_FIND_VALUE_IN_SEPARATOR,
  KV_FIND_VALUE_FINISH,
  KV_FIND_EOL
};

static void
_reset_value(KVScanner *self)
{
  g_string_truncate(self->value, 0);
  self->details.next_key.begin = NULL;
  self->details.next_key.end = NULL;
  self->details.value.begin = self->input + self->input_pos;
  self->details.value.end = self->input + self->input_pos;
  self->value_was_quoted = FALSE;
}

/*
 * Will be different from the original in the future
 */
static inline gboolean
_is_valid_key_character(int c)
{
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') ||
         (c == '_') ||
         (c == '-');
}

static inline void
_dismiss_next_key(KVScanner *self)
{
  if (self->details.next_key.begin < self->details.next_key.end)
    {
      self->details.value.end = self->details.next_key.end;
      self->details.next_key.begin = self->details.next_key.end = NULL;
    }
}

static inline void
_start_next_key(KVScanner *self)
{
  self->details.next_key.begin = self->details.next_key.end = self->input + self->input_pos;
}

static inline void
_end_next_key(KVScanner *self)
{
  self->details.next_key.end = self->input + self->input_pos;
}

static inline void
_start_value(KVScanner *self)
{
  self->details.value.begin = self->details.value.end = self->input + self->input_pos;
}

static inline void
_end_value(KVScanner *self)
{
  self->details.value.end = self->input + self->input_pos;
}

static inline void
_value_init_state(KVScanner *self)
{
  gchar ch = self->input[self->input_pos];

  if (ch == 0)
    {
      self->details.state = KV_FIND_EOL;
    }
  else if (ch == ' ')
    {
      self->details.state = KV_FIND_VALUE_SEPARATOR_AFTER_INIT;
    }
  else if (ch == '\'' || ch == '\"')
    {
      _start_value(self);
      self->quote_char = ch;
      self->details.state = KV_FIND_VALUE_IN_QUOTE;
    }
  else
    {
      _start_value(self);
      self->details.state = KV_FIND_VALUE_VALUE;
    }
}

static inline void
_value_separator_after_init_state(KVScanner *self)
{
  gchar ch = self->input[self->input_pos];

  if (ch == 0)
    {
      self->details.state = KV_FIND_EOL;
    }
  else if (ch == ' ')
    {
      ;
    }
  else if (ch == '\'' || ch == '\"')
    {
      _start_value(self);
      self->quote_char = ch;
      self->details.state = KV_FIND_VALUE_IN_QUOTE;
    }
  else if (!_is_valid_key_character(ch))
    {
      _start_value(self);
      self->details.state = KV_FIND_VALUE_VALUE;
    }
  else
    {
      _start_value(self);
      _start_next_key(self);
      self->details.state = KV_FIND_VALUE_KEY_OR_VALUE;
    }
}

static inline void
_value_value_state(KVScanner *self)
{
  gchar ch = self->input[self->input_pos];

  if (ch == 0)
    {
      _end_value(self);
      self->details.state = KV_FIND_EOL;
    }
  else if (ch == ' ')
    {
      _end_value(self);
      self->details.state = KV_FIND_VALUE_IN_SEPARATOR;
    }
}

static inline void
_value_key_or_value_state(KVScanner *self)
{
  gchar ch = self->input[self->input_pos];

  if (ch == 0)
    {
      _end_next_key(self);
      _dismiss_next_key(self);
      self->details.state = KV_FIND_EOL;
    }
  else if (ch == ' ')
    {
      _end_next_key(self);
      self->details.state = KV_FIND_VALUE_IN_SEPARATOR;
    }
  else if (ch == self->value_separator)
    {
      _end_next_key(self);
      self->details.state = KV_FIND_VALUE_FINISH;
    }
  else if (!_is_valid_key_character(ch))
    {
      _dismiss_next_key(self);
      self->details.state = KV_FIND_VALUE_VALUE;
    }
}

static inline void
_value_in_quote_state(KVScanner *self)
{
  gchar ch = self->input[self->input_pos];

  if (ch == 0)
    {
      _end_value(self);
      self->details.state = KV_FIND_EOL;
    }
  else if (ch == self->quote_char)
    {
      _end_value(self);
      self->details.state = KV_FIND_VALUE_AFTER_QUOTE;
    }
}

static inline void
_value_after_quote_state(KVScanner *self)
{
  gchar ch = self->input[self->input_pos];

  _end_value(self);
  if (ch == 0)
    {
      self->details.state = KV_FIND_EOL;
    }
  else if (ch == ' ')
    {
      self->details.state = KV_FIND_VALUE_IN_SEPARATOR;
    }
  else if (ch == '\'' || ch == '\"')
    {
      self->quote_char = ch;
      self->details.state = KV_FIND_VALUE_IN_QUOTE;
    }
  else if (!_is_valid_key_character(ch))
    {
      self->details.state = KV_FIND_VALUE_VALUE;
    }
  else
    {
      _start_next_key(self);
      self->details.state = KV_FIND_VALUE_KEY_OR_VALUE;
    }
}

static inline void
_value_in_separator_state(KVScanner *self)
{
  gchar ch = self->input[self->input_pos];

  if (ch == 0)
    {
      _dismiss_next_key(self);
      self->details.state = KV_FIND_EOL;
    }
  else if (ch == ' ')
    {
      ;
    }
  else if (ch == self->value_separator)
    {
      self->details.state = KV_FIND_VALUE_FINISH;
    }
  else if (!_is_valid_key_character(ch))
    {
      _dismiss_next_key(self);
      self->details.state = KV_FIND_VALUE_VALUE;
    }
  else
    {
      _dismiss_next_key(self);
      _start_next_key(self);
      self->details.state = KV_FIND_VALUE_KEY_OR_VALUE;
    }
}

gboolean
kv_scanner_generic_extract_value(KVScanner *self)
{
  const gchar *cur;

  _reset_value(self);
  self->details.state = KV_FIND_VALUE_INIT;

  while (self->details.state != KV_FIND_VALUE_FINISH &&
         self->details.state != KV_FIND_EOL)
    {
      switch (self->details.state)
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
      self->input_pos++;
    }

  if (*self->details.value.begin == self->quote_char && *(self->details.value.end-1) == self->quote_char)
    {
      self->value_was_quoted = TRUE;
      self->details.value.begin++;
      self->details.value.end--;
    }

  for (cur = self->details.value.begin; cur < self->details.value.end ; cur++)
    {
      g_string_append_c(self->value, *cur);
    }

  return TRUE;
}

static inline gboolean
_find_first_key(KVScanner *self)
{
  const gchar *cur = &self->input[self->input_pos];
  gboolean found = FALSE;
  self->details.state = KV_FIND_FIRST_KEY_TRIM;

  while (*cur && self->details.state != KV_FIND_FIRST_KEY_FINISH)
    {
      switch (self->details.state)
        {
        case KV_FIND_FIRST_KEY_TRIM:
          if (!_is_valid_key_character(*cur))
            {
              ;
            }
          else
            {
              self->details.next_key.begin = cur;
              self->details.state = KV_FIND_FIRST_KEY_IN_KEY;
            }
          break;

        case KV_FIND_FIRST_KEY_IN_KEY:
          if (*cur == ' ')
            {
              self->details.next_key.end = cur;
              self->details.state = KV_FIND_FIRST_KEY_IN_SEPARATOR;
            }
          else if (*cur == self->value_separator)
            {
              self->details.next_key.end = cur;
              self->details.state = KV_FIND_FIRST_KEY_FINISH;
              found = TRUE;
            }
          else if (!_is_valid_key_character(*cur))
            {
              self->details.next_key.begin = NULL;
              self->details.state = KV_FIND_FIRST_KEY_TRIM;
            }
          break;

        case KV_FIND_FIRST_KEY_IN_SEPARATOR:
          if (*cur == ' ')
            {
              ;
            }
          else if (*cur == self->value_separator)
            {
              self->details.state = KV_FIND_FIRST_KEY_FINISH;
              found = TRUE;
            }
          else if (!_is_valid_key_character(*cur))
            {
              self->details.next_key.begin = NULL;
              self->details.state = KV_FIND_FIRST_KEY_TRIM;
            }
          else
            {
              self->details.next_key.begin = cur;
              self->details.state = KV_FIND_FIRST_KEY_IN_KEY;
            } 
          break;
        }
      cur++;
    }

  self->input_pos = cur - self->input;
  return found;
}

gboolean
kv_scanner_generic_extract_key(KVScanner *self)
{
  KVValueDetails *det = &self->details;

  if (self->details.state == KV_FIND_EOL || self->input[self->input_pos] == 0)
    {
      return FALSE;
    }

  if ((det->next_key.begin == NULL || det->next_key.end == NULL) && !_find_first_key(self))
    {
      return FALSE;
    }

  g_string_assign_len(self->key, self->details.next_key.begin, self->details.next_key.end - self->details.next_key.begin);

  return TRUE;
}