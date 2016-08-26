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
 */
#include "kv-scanner.h"
#include "testutils.h"

#define NULLKV {NULL, NULL}

static gboolean
_assert_no_more_tokens(KVScanner *scanner)
{
  gboolean ok = kv_scanner_scan_next(scanner);

  if (!ok)
  {
    return TRUE;
  }

  GString *msg = g_string_new("kv_scanner is expected to return no more key-value pairs ");
  do
    {
      const gchar *key = kv_scanner_get_current_key(scanner);
      if (!key)
        key = "";
      const gchar *value = kv_scanner_get_current_value(scanner);
      if (!value)
        value = "";
      g_string_append_printf(msg, "[%s/%s]", key, value);
    } while (kv_scanner_scan_next(scanner));
  expect_false(ok, msg->str);
  g_string_free(msg, TRUE);

  return !ok;
}

static gboolean
_assert_current_key_is(KVScanner *scanner, const gchar *expected_key)
{
  const gchar *key = kv_scanner_get_current_key(scanner);

  return expect_nstring(key, -1, expected_key, -1, "current key mismatch");
}

static gboolean
_assert_current_value_is(KVScanner *scanner, const gchar *expected_value)
{
  const gchar *value = kv_scanner_get_current_value(scanner);

  return expect_nstring(value, -1, expected_value, -1, "current value mismatch");
}

static gboolean
_compare_key_value(KVScanner *scanner, const gchar *key, const gchar *value, gboolean *expect_more)
{
  g_assert(value);

  gboolean ok = kv_scanner_scan_next(scanner);
  if (ok)
    {
      _assert_current_key_is(scanner, key);
      _assert_current_value_is(scanner, value);
      return TRUE;
    }
  else
    {
      *expect_more = FALSE;
      expect_true(ok, "kv_scanner is expected to return TRUE for scan_next(), "
                  "first unconsumed pair: [%s/%s]",
                  key, value);
      return FALSE;
    }
}

typedef const gchar *const VAElement;


typedef struct _KVQElement
{
  const gchar *const key;
  const gchar *const value;
  gboolean quoted;
} KVQElement;

typedef struct _KVQContainer
{
  const gsize n;
  const KVQElement *const arg;
} KVQContainer;

#define VARARG_STRUCT(VARARG_STRUCT_cont, VARARG_STRUCT_elem, ...) \
  (const VARARG_STRUCT_cont) { \
    sizeof((const VARARG_STRUCT_elem[]) { __VA_ARGS__ }) / sizeof(VARARG_STRUCT_elem), \
    (const VARARG_STRUCT_elem[]){__VA_ARGS__}\
  }

static void
_scan_kv_pairs_quoted(KVScanner *scanner, const gchar *input, KVQContainer args)
{
  g_assert(input);
  kv_scanner_input(scanner, input);
  gboolean expect_more = TRUE;
  for (gsize i = 0; i < args.n; i++)
    {
      if (!_compare_key_value(scanner, args.arg[i].key, args.arg[i].value, &expect_more))
        break;
      expect_gboolean(scanner->value_was_quoted, args.arg[i].quoted,
                      "mismatch in value_was_quoted for [%s/%s]",
                      args.arg[i].key, args.arg[i].value);
    }
  if (expect_more)
    _assert_no_more_tokens(scanner);
  kv_scanner_free(scanner);
}

#define TEST_KV_SCAN_Q(SCANNER_input, TEST_KV_SCAN_input, ...) \
  do { \
    testcase_begin("TEST_KV_SCAN_Q(%s, %s)", #TEST_KV_SCAN_input, #__VA_ARGS__); \
    _scan_kv_pairs_quoted(SCANNER_input, TEST_KV_SCAN_input, VARARG_STRUCT(KVQContainer, KVQElement, __VA_ARGS__)); \
    testcase_end(); \
  } while (0)

typedef struct _ScannerConfig
{
  gchar kv_separator;
  gboolean allow_pair_separator_in_values;
} ScannerConfig;

typedef struct _KV
{
  gchar* key;
  gchar* value;
} KV;

typedef struct Testcase_t
{
  ScannerConfig config;
  gchar* input;
  KV expected[20];
} Testcase;

static KVScanner*
create_kv_scanner(ScannerConfig config)
{
  KVScanner* new = kv_scanner_new();
  kv_scanner_set_allow_pair_separator_in_values(new, config.allow_pair_separator_in_values);
  kv_scanner_set_value_separator(new, config.kv_separator);
  return new;
}

static void
_scan_kv_pairs_scanner(KVScanner *scanner, const gchar *input, KV kvs[])
{
  g_assert(input);
  kv_scanner_input(scanner, input);
  gboolean expect_more = TRUE;

  while ((*kvs).key)
    {
      if (!(*kvs).value || !_compare_key_value(scanner, (*kvs).key, (*kvs).value, &expect_more))
        break;
      kvs++;
    }
  if (expect_more)
    _assert_no_more_tokens(scanner);
  kv_scanner_free(scanner);
}


static void
_test_key_buffer_underrun()
{
  const gchar *buffer = "ab=v";
  const gchar *input = buffer + 2;

  KV expect_nothing[] = {
    NULLKV
  };

  _scan_kv_pairs_scanner(create_kv_scanner((ScannerConfig){'=', FALSE}), input, expect_nothing);
}

static void
_test_quotation_is_stored_in_the_was_quoted_value_member(void)
{
  ScannerConfig config = {'=', FALSE};
  TEST_KV_SCAN_Q(create_kv_scanner(config), "foo=\"bar\"", {"foo", "bar", TRUE});
  TEST_KV_SCAN_Q(create_kv_scanner(config), "foo='bar'", {"foo", "bar", TRUE});
  TEST_KV_SCAN_Q(create_kv_scanner(config), "foo=bar", {"foo", "bar", FALSE});
  TEST_KV_SCAN_Q(create_kv_scanner(config), "foo='bar", {"foo", "bar", TRUE});
  TEST_KV_SCAN_Q(create_kv_scanner(config), "foo='bar' k=v", {"foo", "bar", TRUE}, {"k", "v", FALSE});
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
_test_transforms_values_if_parse_value_is_set(void)
{
  KVScanner *scanner = create_kv_scanner((ScannerConfig){'=', FALSE});
  scanner->parse_value = _parse_value_by_incrementing_all_bytes;

  _scan_kv_pairs_scanner(scanner, "foo=\"bar\"", (KV[]){ {"foo", "cbs"}, NULLKV });
}

Testcase cases_without_allow_pair_separator_in_value[] = {
  { {'=', FALSE}, "foo=bar", { {"foo", "bar"}, NULLKV } },
  { {'=', FALSE}, "k-j=v", { {"k-j", "v"}, NULLKV } },
  { {'=', FALSE}, "0=v", { {"0", "v"}, NULLKV } },
  { {'=', FALSE}, "_=v", { {"_", "v"}, NULLKV } },
  { {'=', FALSE}, "Z=v", { {"Z", "v"}, NULLKV } },
  { {'=', FALSE}, "k==", { {"k", "="}, NULLKV } },
  { {'=', FALSE}, "k===", { {"k", "=="}, NULLKV } },
  { {'=', FALSE}, "k=\"a", { {"k", "a"}, NULLKV } },
  { {'=', FALSE}, "k=\\", { {"k", "\\"}, NULLKV } },
  { {'=', FALSE}, "k=\"\\", { {"k", ""}, NULLKV } },
  { {'=', FALSE}, "k='a", { {"k", "a"}, NULLKV } },
  { {'=', FALSE}, "k='\\", { {"k", ""}, NULLKV } },
  { {'=', FALSE}, " ==k=", { {"k", ""}, NULLKV } },
  { {'=', FALSE}, " = =k=", { {"k", ""}, NULLKV } },
  { {'=', FALSE}, " =k=", { {"k", ""}, NULLKV } },
  { {'=', FALSE}, " =k=v", { {"k", "v"}, NULLKV } },
  { {'=', FALSE}, " ==k=v", { {"k", "v"}, NULLKV } },
  { {'=', FALSE}, " =k=v=w", { {"k", "v=w"}, NULLKV } },
  { {'=', FALSE}, "k=\xc3", { {"k", "\xc3"}, NULLKV } },
  { {'=', FALSE}, "k=\xc3v", { {"k", "\xc3v"}, NULLKV } },
  { {'=', FALSE}, "k=\xff", { {"k", "\xff"}, NULLKV } },
  { {'=', FALSE}, "k=\xffv", { {"k", "\xffv"}, NULLKV } },
  { {'=', FALSE}, "k=\"\xc3v", { {"k", "\xc3v"}, NULLKV } },
  { {'=', FALSE}, "k=\"\xff", { {"k", "\xff"}, NULLKV } },
  { {'=', FALSE}, "k=\"\xffv", { {"k", "\xffv"}, NULLKV } },
  { {'=', FALSE}, "foo=", { {"foo", ""}, NULLKV } },
  { {'=', FALSE}, "foo=b", { {"foo", "b"}, NULLKV } },
  { {'=', FALSE}, "lorem ipsum foo=bar", { {"foo", "bar"}, NULLKV } },
  { {'=', FALSE}, "lorem ipsum/dolor @sitamen foo=bar", { {"foo", "bar"}, NULLKV } },
  { {'=', FALSE}, "lorem ipsum/dolor = foo=bar", { {"foo", "bar"}, NULLKV } },
  { {'=', FALSE}, "*k=v", { {"k", "v"}, NULLKV } },
  { {'=', FALSE}, "x *k=v", { {"k", "v"}, NULLKV } },
  { {'=', FALSE}, "k1=v1 k2=v2 k3=v3", { {"k1", "v1"}, {"k2", "v2"}, {"k3", "v3"}, NULLKV } },
  { {'=', FALSE}, "k1=v1    k2=v2     k3=v3 ", { {"k1", "v1"}, {"k2", "v2"}, {"k3", "v3"}, NULLKV } },
  { {'=', FALSE}, "k1=v1,k2=v2,k3=v3", { {"k1", "v1,k2=v2,k3=v3"}, NULLKV } },
  { {'=', FALSE}, "k1=v1\tk2=v2 k3=v3", { {"k1", "v1\tk2=v2"}, {"k3", "v3"}, NULLKV } },
  { {'=', FALSE}, "k1=v1,\tk2=v2 k3=v3", { {"k1", "v1,\tk2=v2"}, {"k3", "v3"}, NULLKV } },
  { {'=', FALSE}, "k1=v1\t k2=v2 k3=v3", { {"k1", "v1\t"}, {"k2", "v2"}, {"k3", "v3"}, NULLKV } },
  { {'=', FALSE}, "k=\t", { {"k", "\t"}, NULLKV } },
  { {'=', FALSE}, "k=,\t", { {"k", ",\t"}, NULLKV } },
  { {'=', FALSE}, "foo=\"bar\"", { {"foo", "bar"}, NULLKV } },
  { {'=', FALSE}, "k1=\"v1\", k2=\"v2\"", { {"k1", "v1"}, {"k2", "v2"}, NULLKV } },
  { {'=', FALSE}, "k1=\"\\\"v1\"", { {"k1", "\"v1"}, NULLKV } },
  { {'=', FALSE}, "k1=\"\\b \\f \\n \\r \\t \\\\\"", { {"k1", "\b \f \n \r \t \\"}, NULLKV } },
  { {'=', FALSE}, "k1=\"\\p\"", { {"k1", "\\p"}, NULLKV } },
  { {'=', FALSE}, "k1='\\'v1'", { {"k1", "'v1"}, NULLKV } },
  { {'=', FALSE}, "k1='\\b \\f \\n \\r \\t \\\\'", { {"k1", "\b \f \n \r \t \\"}, NULLKV } },
  { {'=', FALSE}, "k1='\\p'", { {"k1", "\\p"}, NULLKV } },
  { {'=', FALSE}, "k1=\\b\\f\\n\\r\\t\\\\", { {"k1", "\\b\\f\\n\\r\\t\\\\"}, NULLKV } },
  { {'=', FALSE}, "k1=\b\f\n\r\\", { {"k1", "\b\f\n\r\\"}, NULLKV } },
  { {'=', FALSE}, "k1=\"v foo, foo2 =@,\\\"\" k2='v foo,  a='",
      { {"k1", "v foo, foo2 =@,\""}, {"k2", "v foo,  a="}, NULLKV } },
  { {'=', FALSE}, "k1=v1, k2=v2, k3=v3", { {"k1", "v1"}, {"k2", "v2"}, {"k3", "v3"}, NULLKV } },
  { {'=', FALSE}, "foo=bar lorem ipsum key=value some more values",
      { {"foo", "bar"}, {"key", "value"}, NULLKV } },
  { {'=', FALSE}, "k1=v1,   k2=v2  ,    k3=v3", { {"k1", "v1"}, {"k2", "v2"}, {"k3", "v3"}, NULLKV } },
  { {'=', FALSE}, "k1 k2=v2, k3, k4=v4", { {"k2", "v2"}, {"k4", "v4"}, NULLKV } },
  { {'=', FALSE}, "k1= k2=v2, k3=, k4=v4 k5= , k6=v6",
      { {"k1", ""}, {"k2", "v2"}, {"k3", ""}, {"k4", "v4"}, {"k5", ""}, {"k6", "v6"}, NULLKV } },
  { {'=', FALSE}, "k1= v1 k2 = v2 k3 =v3 ", { {"k1", ""}, NULLKV } },
  { {'=', FALSE}, "k1='v1', k2='v2'", { {"k1", "v1"}, {"k2", "v2"}, NULLKV } },
  { {'=', FALSE}, ", k=v", { {"k", "v"}, NULLKV } },
  { {'=', FALSE}, ",k=v", { {"k", "v"}, NULLKV } },
  { {'=', FALSE}, "k=v,", { {"k", "v,"}, NULLKV } },
  { {'=', FALSE}, "k=v, ", { {"k", "v"}, NULLKV } },
  { {':', FALSE}, "k1:v1 k2:v2 k3:v3 ", { {"k1", "v1"}, {"k2", "v2"}, {"k3", "v3"}, NULLKV } },
  { {'-', FALSE}, "k-v", { {"k", "v"}, NULLKV } },
  { {'-', FALSE}, "k--v", { {"k", "-v"}, NULLKV } },
  { {'-', FALSE}, "---", { {"-", "-"}, NULLKV } },
  { {'=', FALSE}, "=v", { NULLKV } },
  { {'=', FALSE}, "k*=v", { NULLKV } },
  { {'=', FALSE}, "=", { NULLKV } },
  { {'=', FALSE}, "==", { NULLKV } },
  { {'=', FALSE}, "===", { NULLKV } },
  { {'=', FALSE}, " =", { NULLKV } },
  { {'=', FALSE}, " ==", { NULLKV } },
  { {'=', FALSE}, " ===", { NULLKV } },
  { {'=', FALSE}, " = =", { NULLKV } },
  { {'=', FALSE}, ":=", { NULLKV } },
  { {'=', FALSE}, "á=v", { NULLKV } },
  { {'=', FALSE}, "", { NULLKV } },
  { {'=', FALSE}, "f", { NULLKV } },
  { {'=', FALSE}, "fo", { NULLKV } },
  { {'=', FALSE}, "foo", { NULLKV } },

  { {0, FALSE  }, NULL,      { NULLKV                 } }
};

gchar*
_expected_to_string(KV* kvs)
{
  gchar* result = "";
  if ((*kvs).key)
  {
    result = (*kvs).key;
    result = g_strjoin("=", result, (*kvs).value, NULL);
    kvs++;
  }
  while ((*kvs).key)
  {
    result = g_strjoin(" ", result, (*kvs).key, NULL);
    result = g_strjoin("=", result, (*kvs).value, NULL);
    kvs++;
  }

  return result;
}

void
_run_testcase(Testcase tc)
{
  testcase_begin("input:(%s), expected:(%s)", tc.input, _expected_to_string(tc.expected));
  _scan_kv_pairs_scanner(create_kv_scanner(tc.config), tc.input, tc.expected);
  testcase_end();
}

void
_run_testcases(Testcase* cases)
{
  Testcase* tc = cases;
  while (tc->input)
  {
    _run_testcase(*tc);
    tc++;
  }
}

int main(int argc, char *argv[])
{
  _test_quotation_is_stored_in_the_was_quoted_value_member();
  _test_key_buffer_underrun();
  _test_transforms_values_if_parse_value_is_set();
  _run_testcases(cases_without_allow_pair_separator_in_value);
  if (testutils_deinit())
    return 0;
  else
    return 1;
}
