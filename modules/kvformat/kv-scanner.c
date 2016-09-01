/*
 * Copyright (c) 2015-2016 Balabit
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
#include "kv-scanner.h"
#include "misc.h"
#include "utf8utils.h"

#include <string.h>

enum
{
  KV_QUOTE_INITIAL = 0,
  KV_QUOTE_STRING,
  KV_QUOTE_BACKSLASH,
  KV_QUOTE_FINISH
};

enum {
  KV_FIND_FIRST_KEY_TRIM = 0,
  KV_FIND_FIRST_KEY_IN_KEY,
  KV_FIND_FIRST_KEY_IN_SEPARATOR,
  KV_FIND_FIRST_KEY_FINISH,
  KV_FIND_VALUE_INIT,
  KV_FIND_VALUE_VALUE,
  KV_FIND_VALUE_KEY_OR_VALUE,
  KV_FIND_VALUE_IN_QUOTE,
  KV_FIND_VALUE_AFTER_QUOTE,
  KV_FIND_VALUE_IN_SEPARATOR,
  KV_FIND_VALUE_FINISH,
  KV_FIND_EOL
};

void
kv_scanner_allow_pair_separator_in_value(KVScanner *self, gboolean allowed)
{
  self->allow_pair_separator_in_value = allowed;
}

void
kv_scanner_set_value_separator(KVScanner *self, gchar value_separator)
{
  self->value_separator = value_separator;
}

void
kv_scanner_input(KVScanner *self, const gchar *input)
{
  self->input = input;
  self->input_len = strlen(input);
  self->input_pos = 0;
}

static void
_kv_scanner_reset_value(KVScanner *self)
{
  g_string_truncate(self->value, 0);
  self->details.next_key.begin = NULL;
  self->details.next_key.end = NULL;
  self->details.value.begin = self->input + self->input_pos;
  self->details.value.end = self->input + self->input_pos;
}

static gboolean
_kv_scanner_skip_space(KVScanner *self)
{
  while (self->input[self->input_pos] == ' ')
    self->input_pos++;
  return TRUE;
}

static gboolean
_is_valid_key_character(int c)
{
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') ||
         (c == '_') ||
         (c == '-');
}

static gboolean
_kv_scanner_extract_key(KVScanner *self)
{
  const gchar *input_ptr = &self->input[self->input_pos];
  const gchar *start_of_key;
  const gchar *separator;
  gsize len;

  separator = strchr(input_ptr, self->value_separator);
  do
    {
      if (!separator)
        return FALSE;
      start_of_key = separator;
      while (start_of_key > input_ptr && _is_valid_key_character(*(start_of_key - 1)))
        start_of_key--;
      len = separator - start_of_key;
      if (len < 1)
        separator = strchr(separator + 1, self->value_separator);
    }
  while (len < 1);

  g_string_assign_len(self->key, start_of_key, len);
  self->input_pos = separator - self->input + 1;
  return TRUE;
}

static void
_decode_backslash_escape(KVScanner *self, gchar ch)
{
  gchar control;
  switch (ch)
    {
      case 'b':
        control = '\b';
        break;
      case 'f':
        control = '\f';
        break;
      case 'n':
        control = '\n';
        break;
      case 'r':
        control = '\r';
        break;
      case 't':
        control = '\t';
        break;
      case '\\':
        control = '\\';
        break;
      default:
        if (self->quote_char != ch)
          g_string_append_c(self->value, '\\');
          control = ch;
        break;
    }
  g_string_append_c(self->value, control);
}

static gboolean
_is_delimiter(const gchar *cur)
{
  return (*cur == ' ') || (strncmp(cur, ", ", 2) == 0);
}

static gboolean
_kv_scanner_extract_value(KVScanner *self)
{
  const gchar *cur;

  _kv_scanner_reset_value(self);
  self->value_was_quoted = FALSE;
  cur = &self->input[self->input_pos];

  self->quote_state = KV_QUOTE_INITIAL;
  while (*cur && self->quote_state != KV_QUOTE_FINISH)
    {
      switch (self->quote_state)
        {
        case KV_QUOTE_INITIAL:
          if (_is_delimiter(cur))
            {
              self->quote_state = KV_QUOTE_FINISH;
            }
          else if (*cur == '\"' || *cur == '\'')
            {
              self->quote_state = KV_QUOTE_STRING;
              self->quote_char = *cur;
              if (self->value->len == 0)
                self->value_was_quoted = TRUE;
            }
          else
            {
              g_string_append_c(self->value, *cur);
            }
          break;
        case KV_QUOTE_STRING:
          if (*cur == self->quote_char)
            self->quote_state = KV_QUOTE_INITIAL;
          else if (*cur == '\\')
            self->quote_state = KV_QUOTE_BACKSLASH;
          else
            g_string_append_c(self->value, *cur);
          break;
        case KV_QUOTE_BACKSLASH:
          _decode_backslash_escape(self, *cur);
          self->quote_state = KV_QUOTE_STRING;
          break;
        }
      cur++;
    }
  self->input_pos = cur - self->input;
  return TRUE;
}

static gboolean
_kv_scanner_decode_value(KVScanner *self)
{
  if (self->parse_value)
    {
      g_string_truncate(self->decoded_value, 0);
      if (self->parse_value(self))
        g_string_assign_len(self->value, self->decoded_value->str, self->decoded_value->len);
    }
  return TRUE;
}

void
_dismiss_next_key(KVScanner *self)
{
  if (self->details.next_key.begin < self->details.next_key.end)
    {
      self->details.value.end = self->details.next_key.end;
      self->details.next_key.begin = self->details.next_key.end = NULL;
    }
}

void
_start_next_key(KVScanner *self)
{
  self->details.next_key.begin = self->details.next_key.end = self->input + self->input_pos;
}

void
_end_next_key(KVScanner *self)
{
  self->details.next_key.end = self->input + self->input_pos;
}

void
_start_value(KVScanner *self)
{
  self->details.value.begin = self->details.value.end = self->input + self->input_pos;
}

void
_end_value(KVScanner *self)
{
  self->details.value.end = self->input + self->input_pos;
}

void
handle_key_or_value_state(KVScanner *self)
{
  const gchar *cur = self->input + self->input_pos;

  if (*cur == 0)
    {
      _end_next_key(self);
      _dismiss_next_key(self);
      self->details.state = KV_FIND_EOL;
    }
  else if (*cur == ' ')
    {
      _end_next_key(self);
      self->details.state = KV_FIND_VALUE_IN_SEPARATOR;
    }
  else if (*cur == self->value_separator)
    {
      _end_next_key(self);
      self->details.state = KV_FIND_VALUE_FINISH;
    }
  else if (!_is_valid_key_character(*cur))
    {
      _dismiss_next_key(self);
      self->details.state = KV_FIND_VALUE_VALUE;
    }
}

void
handle_value_state(KVScanner *self)
{
  const gchar *cur = self->input + self->input_pos;

  if (*cur == 0)
    {
      _end_value(self);
      self->details.state = KV_FIND_EOL;
    }
  else if (*cur == ' ')
    {
      _end_value(self);
      self->details.state = KV_FIND_VALUE_IN_SEPARATOR;
    }
}

void
handle_in_separator_state(KVScanner *self)
{
  const gchar *cur = self->input + self->input_pos;

  if (*cur == 0)
    {
      _dismiss_next_key(self);
      self->details.state = KV_FIND_EOL;
    }
  else if (*cur == ' ')
    {
      ;
    }
  else if (*cur == self->value_separator)
    {
      self->details.state = KV_FIND_VALUE_FINISH;
    }
  else if (!_is_valid_key_character(*cur))
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

void
_handle_init_state(KVScanner *self)
{
  const gchar *cur = self->input + self->input_pos;

  if (*cur == 0)
    {
      self->details.state = KV_FIND_EOL;
    }
  else if (*cur == ' ')
    {
      ;
    }
  else if (*cur == '\'' || *cur == '\"')
    {
      _start_value(self);
      self->quote_char = *cur;
      self->details.state = KV_FIND_VALUE_IN_QUOTE;
    }
  else if (!_is_valid_key_character(*cur))
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

void
handle_after_quote_state(KVScanner *self)
{
  const gchar *cur = self->input + self->input_pos;

  _end_value(self);
  if (*cur == 0)
    {
      self->details.state = KV_FIND_EOL;
    }
  else if (*cur == ' ')
    {
      self->details.state = KV_FIND_VALUE_IN_SEPARATOR;
    }
  else if (*cur == '\'' || *cur == '\"')
    {
      self->quote_char = *cur;
      self->details.state = KV_FIND_VALUE_IN_QUOTE;
    }
  else if (!_is_valid_key_character(*cur))
    {
      self->details.state = KV_FIND_VALUE_VALUE;
    }
  else
    {
      _start_next_key(self);
      self->details.state = KV_FIND_VALUE_KEY_OR_VALUE;
    }
}

void
handle_in_quote_state(KVScanner *self)
{
  const gchar *cur = self->input + self->input_pos;

  if (*cur == 0)
    {
      _end_value(self);
      self->details.state = KV_FIND_VALUE_FINISH;
    }
  else if (*cur == self->quote_char)
    {
      _end_value(self);
      self->details.state = KV_FIND_VALUE_AFTER_QUOTE;
    }
}

gboolean
_kv_scanner_extract_value_new(KVScanner *self)
{
  const gchar *cur;

  _kv_scanner_reset_value(self);
  //
  self->details.state = KV_FIND_VALUE_INIT;

  while (self->details.state != KV_FIND_VALUE_FINISH &&
         self->details.state != KV_FIND_EOL) {
    switch (self->details.state) {
      case KV_FIND_VALUE_INIT:
        _handle_init_state(self);
        break;
      case KV_FIND_VALUE_VALUE:
        handle_value_state(self);
        break;
      case KV_FIND_VALUE_KEY_OR_VALUE:
        handle_key_or_value_state(self);
        break;
      case KV_FIND_VALUE_IN_SEPARATOR:
        handle_in_separator_state(self);
        break;
      case KV_FIND_VALUE_IN_QUOTE:
        handle_in_quote_state(self);
        break;
      case KV_FIND_VALUE_AFTER_QUOTE:
        handle_after_quote_state(self);
        break;
    }
    self->input_pos++;
  }

  if (*self->details.value.begin == self->quote_char && *(self->details.value.end-1) == self->quote_char) {
    self->details.value.begin++;
    self->details.value.end--;
  }

  for (cur = self->details.value.begin; cur < self->details.value.end ; cur++) {
    g_string_append_c(self->value, *cur);
  }

  return TRUE;
}

static gboolean
_find_first_key(KVScanner *self)
{
  const gchar *cur = &self->input[self->input_pos];
  gboolean found = FALSE;
  self->details.state = KV_FIND_FIRST_KEY_TRIM;

  while (*cur && self->details.state != KV_FIND_FIRST_KEY_FINISH) {
    switch (self->details.state) {

      case KV_FIND_FIRST_KEY_TRIM:
        if (!_is_valid_key_character(*cur)) {
          ;
        } else {
          self->details.next_key.begin = cur;
          self->details.state = KV_FIND_FIRST_KEY_IN_KEY;
        }
        break;

      case KV_FIND_FIRST_KEY_IN_KEY:
        if (*cur == ' ') {
          self->details.next_key.end = cur;
          self->details.state = KV_FIND_FIRST_KEY_IN_SEPARATOR;
        } else if (*cur == self->value_separator) {
          self->details.next_key.end = cur;
          self->details.state = KV_FIND_FIRST_KEY_FINISH;
          found = TRUE;
        } else if (!_is_valid_key_character(*cur)) {
          self->details.next_key.begin = NULL;
          self->details.state = KV_FIND_FIRST_KEY_TRIM;
        }
        break;

      case KV_FIND_FIRST_KEY_IN_SEPARATOR:
        if (*cur == ' ') {
          ;
        } else if (*cur == self->value_separator) {
          self->details.state = KV_FIND_FIRST_KEY_FINISH;
          found = TRUE;
        } else if (!_is_valid_key_character(*cur)) {
          self->details.next_key.begin = NULL;
          self->details.state = KV_FIND_FIRST_KEY_TRIM;
        } else {
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
_kv_scanner_extract_key_new(KVScanner *self)
{
  KVValueDetails *det = &self->details;

  if (self->input[self->input_pos] == 0)
    return FALSE;

  if (det->next_key.begin == NULL ||
      det->next_key.end == NULL ) {
    if (!_find_first_key(self))
      return FALSE;
  }
  g_string_assign_len(self->key, self->details.next_key.begin, self->details.next_key.end - self->details.next_key.begin);

  return TRUE;
}

gboolean
_kv_scanner_finished(KVScanner *self)
{
  return self->details.state == KV_FIND_EOL;
}

gboolean
kv_scanner_scan_next(KVScanner *self)
{
  _kv_scanner_skip_space(self);

  if (self->allow_pair_separator_in_value) {
    if (_kv_scanner_finished(self) ||
        !_kv_scanner_extract_key_new(self) ||
        !_kv_scanner_extract_value_new(self))
      return FALSE;
  } else {
    if (!_kv_scanner_extract_key(self) ||
        !_kv_scanner_extract_value(self) ||
        !_kv_scanner_decode_value(self))
    return FALSE;
  }
  return TRUE;
}

const gchar *
kv_scanner_get_current_key(KVScanner *self)
{
  return self->key->str;
}

const gchar *
kv_scanner_get_current_value(KVScanner *self)
{
  return self->value->str;
}

void
kv_scanner_free_method(KVScanner *self)
{
  g_string_free(self->key, TRUE);
  g_string_free(self->value, TRUE);
  g_string_free(self->decoded_value, TRUE);
}

/* NOTE: this is a very limited clone operation that doesn't allow
 * descendant types (e.g.  linux-audit scanner to have their own state */
KVScanner *
kv_scanner_clone(KVScanner *self)
{
  KVScanner *cloned = kv_scanner_new();
  cloned->parse_value = self->parse_value;
  cloned->value_separator = self->value_separator;
  cloned->allow_pair_separator_in_value = self->allow_pair_separator_in_value;
  return cloned;
}

void
kv_scanner_init(KVScanner *self)
{
  memset(self, 0, sizeof(*self));
  self->key = g_string_sized_new(32);
  self->value = g_string_sized_new(64);
  self->decoded_value = g_string_sized_new(64);
  self->free_fn = kv_scanner_free_method;
  self->value_separator = '=';
  self->allow_pair_separator_in_value = FALSE;
}

KVScanner *
kv_scanner_new(void)
{
  KVScanner *self = g_new0(KVScanner, 1);

  kv_scanner_init(self);
  return self;
}

void
kv_scanner_free(KVScanner *self)
{
  if (self->free_fn)
    self->free_fn(self);
  g_free(self);
}
