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
#include "kv-scanner.h"
#include "testutils.h"
#include <stdarg.h>

#define kv_scanner_testcase_begin(func, args)             \
  do                                                            \
    {                                                           \
      testcase_begin("%s(%s)", func, args);                     \
    }                                                           \
  while (0)

#define kv_scanner_testcase_end()                           \
  do                                                            \
    {                                                           \
      testcase_end();                                           \
    }                                                           \
  while (0)

#define KV_SCANNER_TESTCASE(x, ...) \
  do {                                                          \
      kv_scanner_testcase_begin(#x, #__VA_ARGS__);  		\
      x(__VA_ARGS__);                                           \
      kv_scanner_testcase_end();                                \
  } while(0)

static void
assert_no_more_tokens(KVScanner *scanner)
{
  assert_false(kv_scanner_scan_next(scanner), "kv_scanner is expected to return no more key-value pairs");
}

static void
scan_next_token(KVScanner *scanner)
{
  assert_true(kv_scanner_scan_next(scanner),  "kv_scanner is expected to return TRUE for scan_next");
}

static void
assert_current_key_is(KVScanner *scanner, const gchar *expected_key)
{
  const gchar *key = kv_scanner_get_current_key(scanner);

  assert_string(key, expected_key, "current key mismatch");
}

static void
assert_current_value_is(KVScanner *scanner, const gchar *expected_value)
{
  const gchar *value = kv_scanner_get_current_value(scanner);

  assert_string(value, expected_value, "current value mismatch");
}

static void
assert_next_kv_is(KVScanner *scanner, const gchar *expected_key, const gchar *expected_value)
{
  scan_next_token(scanner);
  assert_current_key_is(scanner, expected_key);
  assert_current_value_is(scanner, expected_value);
}

static void
_scan_kv_pairs(KVScanner *scanner, const gchar *input, va_list args)
{
  g_assert(input);

  kv_scanner_input(scanner, input);
  const gchar *kv;
  do {
      kv = va_arg(args, const gchar *);
      if (!kv)
        break;

      scan_next_token(scanner);

      assert_current_key_is(scanner, kv);

      kv = va_arg(args, const gchar *);
      if (kv)
        assert_current_value_is(scanner, kv);
    } while (kv);
  assert_no_more_tokens(scanner);
}

static void
_scan_kv_pairs_scanner(KVScanner *scanner, const gchar *input, ...)
{
  va_list args;
  va_start(args, input);
  _scan_kv_pairs(scanner, input, args);
  va_end(args);
}

static void
_scan_kv_pairs_auto(const gchar *input, ...)
{
  KVScanner *scanner = kv_scanner_new();
  va_list args;
  va_start(args, input);
  _scan_kv_pairs(scanner, input, args);
  va_end(args);
  kv_scanner_free(scanner);
}

#define TEST_KV_SCAN(input, ...) \
  _scan_kv_pairs_auto(input, ##__VA_ARGS__, NULL);
#define TEST_KV_SCANNER(scanner, input, ...) \
  _scan_kv_pairs_scanner(scanner, input, ##__VA_ARGS__, NULL);

static void
test_kv_scanner_incomplete_string_returns_no_pairs(void)
{
  TEST_KV_SCAN("");
  TEST_KV_SCAN("f");
  TEST_KV_SCAN("fo");
  TEST_KV_SCAN("foo");
}

static void
test_kv_scanner_name_equals_value_returns_a_pair(void)
{
  TEST_KV_SCAN("foo=", "foo", "");
  TEST_KV_SCAN("foo=b", "foo", "b");
  TEST_KV_SCAN("foo=bar", "foo", "bar");
  TEST_KV_SCAN("foo=barbar", "foo", "barbar");
}

static void
test_kv_scanner_stray_words_are_ignored(void)
{
  TEST_KV_SCAN("lorem ipsum foo=bar", "foo", "bar");
  TEST_KV_SCAN("lorem ipsum/dolor @sitamen foo=bar", "foo", "bar");
  TEST_KV_SCAN("lorem ipsum/dolor = foo=bar\"", "", "", "foo", "bar");
  TEST_KV_SCAN("foo=bar lorem ipsum key=value some more values", "foo", "bar", "key", "value");
}

static void
test_kv_scanner_with_multiple_key_values_return_multiple_pairs(void)
{
  TEST_KV_SCAN("key1=value1 key2=value2 key3=value3 ",
               "key1", "value1", "key2", "value2", "key3", "value3");
}

static void
test_kv_scanner_spaces_between_values_are_ignored(void)
{
  TEST_KV_SCAN("key1=value1    key2=value2     key3=value3 ",
               "key1", "value1", "key2", "value2", "key3", "value3");
}

static void
test_kv_scanner_with_comma_separated_values(void)
{
  TEST_KV_SCAN("key1=value1, key2=value2, key3=value3",
               "key1", "value1", "key2", "value2", "key3", "value3");
}

static void
test_kv_scanner_with_comma_separated_values_and_multiple_spaces(void)
{
  TEST_KV_SCAN("key1=value1,   key2=value2  ,    key3=value3",
               "key1", "value1",
               "key2", "value2",
               "key3", "value3");
}

static void
test_kv_scanner_with_comma_separated_values_without_space(void)
{
  TEST_KV_SCAN("key1=value1,key2=value2,key3=value3",
               "key1", "value1,key2=value2,key3=value3");
}

static void
test_kv_scanner_tab_separated_values(void)
{
  TEST_KV_SCAN("key1=value1\tkey2=value2 key3=value3",
               "key1", "value1\tkey2=value2",
               "key3", "value3");
}

static void
test_kv_scanner_quoted_values_are_unquoted_like_c_strings(void)
{
  TEST_KV_SCAN("foo=\"bar\"", "foo", "bar");

  TEST_KV_SCAN("key1=\"value1\", key2=\"value2\"", "key1", "value1", "key2", "value2");

  /* embedded quote */
  TEST_KV_SCAN("key1=\"\\\"value1\"", "key1", "\"value1");

  /* control sequences */
  TEST_KV_SCAN("key1=\"\\b \\f \\n \\r \\t \\\\\"",
               "key1", "\b \f \n \r \t \\");

  /* unknown backslash escape is left as is */
  TEST_KV_SCAN("key1=\"\\p\"",
               "key1", "\\p");

  TEST_KV_SCAN("key1='value1', key2='value2'",
               "key1", "value1",
               "key2", "value2");

  /* embedded quote */
  TEST_KV_SCAN("key1='\\'value1'", "key1", "'value1");

  /* control sequences */
  TEST_KV_SCAN("key1='\\b \\f \\n \\r \\t \\\\'",
               "key1", "\b \f \n \r \t \\");

  /* unknown backslash escape is left as is */
  TEST_KV_SCAN("key1='\\p'", "key1", "\\p");
}

static void
test_kv_scanner_keys_without_values(void)
{
  TEST_KV_SCAN("key1 key2=value2, key3, key4=value4",
               "key2", "value2",
               "key4", "value4");

  TEST_KV_SCAN("key1= key2=value2, key3=, key4=value4 key5= , key6=value6",
               "key1", "",
               "key2", "value2",
               "key3", "",
               "key4", "value4",
               "key5", "",
               "key6", "value6");
}

static void
test_kv_scanner_quoted_values_with_special_characters(void)
{
  TEST_KV_SCAN("key1=\"value foo, foo2 =@,\\\"\" key2='value foo,  a='",
               "key1", "value foo, foo2 =@,\"",
               "key2", "value foo,  a=");
}

static gboolean
_parse_value_by_incrementing_all_bytes(KVScanner *self)
{
  gint i;

  g_string_assign(self->decoded_value, self->value->str);
  for (i = 0; i < self->decoded_value->len; i++)
    self->decoded_value->str[i]++;
  return TRUE;
}

static void
test_kv_scanner_transforms_values_if_parse_value_is_set(void)
{
  KVScanner *scanner = kv_scanner_new();
  scanner->parse_value = _parse_value_by_incrementing_all_bytes;
  TEST_KV_SCANNER(scanner, "foo=\"bar\"", "foo", "cbs");
  kv_scanner_free(scanner);
}

static void
test_kv_scanner_quotation_is_stored_in_the_was_quoted_value_member(void)
{
  KVScanner *scanner = kv_scanner_new();
  kv_scanner_input(scanner, "foo=\"bar\"");
  assert_next_kv_is(scanner, "foo", "bar");
  assert_true(scanner->value_was_quoted, "expected value_was_quoted to be TRUE");
  assert_no_more_tokens(scanner);

  kv_scanner_input(scanner, "foo=bar");
  assert_next_kv_is(scanner, "foo", "bar");
  assert_false(scanner->value_was_quoted, "expected value_was_quoted to be FALSE");
  assert_no_more_tokens(scanner);
  kv_scanner_free(scanner);
}

static void
test_kv_scanner_value_separator_with_whitespaces_around(void)
{
  KVScanner *scanner = kv_scanner_new();
  kv_scanner_set_value_separator(scanner, ':');

  TEST_KV_SCANNER(scanner, "key1: value1 key2 : value2 key3 :value3 ",
                  "key1", "",
                  "", "",
                  "", "value3");

  kv_scanner_free(scanner);
}

static void
test_kv_scanner_value_separator_is_used_to_separate_key_from_value(void)
{
  KVScanner *scanner = kv_scanner_new();
  kv_scanner_set_value_separator(scanner, ':');

  TEST_KV_SCANNER(scanner, "key1:value1 key2:value2 key3:value3 ",
                  "key1", "value1",
                  "key2", "value2",
                  "key3", "value3");

  kv_scanner_free(scanner);
}

static void
test_kv_scanner_value_separator_clone(void)
{
  KVScanner *scanner = kv_scanner_new();
  kv_scanner_set_value_separator(scanner, ':');
  KVScanner *cloned_scanner = kv_scanner_clone(scanner);
  kv_scanner_free(scanner);

  TEST_KV_SCANNER(cloned_scanner, "key1:value1 key2:value2 key3:value3 ",
                  "key1", "value1",
                  "key2", "value2",
                  "key3", "value3");
  kv_scanner_free(cloned_scanner);
}

static void
test_kv_scanner(void)
{
  KV_SCANNER_TESTCASE(test_kv_scanner_incomplete_string_returns_no_pairs);
  KV_SCANNER_TESTCASE(test_kv_scanner_stray_words_are_ignored);
  KV_SCANNER_TESTCASE(test_kv_scanner_name_equals_value_returns_a_pair);
  KV_SCANNER_TESTCASE(test_kv_scanner_with_multiple_key_values_return_multiple_pairs);
  KV_SCANNER_TESTCASE(test_kv_scanner_spaces_between_values_are_ignored);
  KV_SCANNER_TESTCASE(test_kv_scanner_with_comma_separated_values);
  KV_SCANNER_TESTCASE(test_kv_scanner_with_comma_separated_values_and_multiple_spaces);
  KV_SCANNER_TESTCASE(test_kv_scanner_with_comma_separated_values_without_space);
  KV_SCANNER_TESTCASE(test_kv_scanner_tab_separated_values);
  KV_SCANNER_TESTCASE(test_kv_scanner_quoted_values_are_unquoted_like_c_strings);
  KV_SCANNER_TESTCASE(test_kv_scanner_keys_without_values);
  KV_SCANNER_TESTCASE(test_kv_scanner_quoted_values_with_special_characters);
  KV_SCANNER_TESTCASE(test_kv_scanner_transforms_values_if_parse_value_is_set);
  KV_SCANNER_TESTCASE(test_kv_scanner_quotation_is_stored_in_the_was_quoted_value_member);
  KV_SCANNER_TESTCASE(test_kv_scanner_value_separator_is_used_to_separate_key_from_value);
  KV_SCANNER_TESTCASE(test_kv_scanner_value_separator_clone);
  KV_SCANNER_TESTCASE(test_kv_scanner_value_separator_with_whitespaces_around);
}

int main(int argc, char *argv[])
{
  test_kv_scanner();
}
