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

static void
_assert_no_more_tokens(KVScanner *scanner)
{
  gboolean ok = kv_scanner_scan_next(scanner);

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
_compare_key_value(KVScanner *scanner, const gchar *key, const gchar *value)
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
  for (gsize i = 0; i < args.n; i++)
    {
      if (!_compare_key_value(scanner, args.arg[i].key, args.arg[i].value))
        break;
      expect_gboolean(scanner->value_was_quoted, args.arg[i].quoted,
                      "mismatch in value_was_quoted for [%s/%s]",
                      args.arg[i].key, args.arg[i].value);
    }
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

  while (kvs->key)
    {
      if (!kvs->value || !_compare_key_value(scanner, kvs->key, kvs->value))
        break;
      kvs++;
    }
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

static void
_test_value_separator_clone(void)
{
  KVScanner *scanner = kv_scanner_new();
  kv_scanner_set_value_separator(scanner, ':');
  KVScanner *cloned_scanner = kv_scanner_clone(scanner);
  kv_scanner_free(scanner);

  _scan_kv_pairs_scanner(
    cloned_scanner,
    "key1:value1 key2:value2 key3:value3 ",
    (KV[]){{"key1", "value1"}, {"key2", "value2"}, {"key3", "value3"}, NULLKV}
  );
}

#define DEFAULT_CONFIG {.kv_separator='=', .allow_pair_separator_in_values=FALSE}

static Testcase*
provide_cases_without_allow_pair_separator_in_value()
{
  static Testcase cases_without_allow_pair_separator_in_value[] = {
    {
      DEFAULT_CONFIG,
      "foo=bar",
      { {"foo", "bar"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k-j=v",
      { {"k-j", "v"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "0=v",
      { {"0", "v"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "_=v",
      { {"_", "v"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "Z=v",
      { {"Z", "v"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k==",
      { {"k", "="}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k===",
      { {"k", "=="}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k=\"a",
      { {"k", "a"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k=\\",
      { {"k", "\\"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k=\"\\",
      { {"k", ""}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k='a",
      { {"k", "a"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k='\\",
      { {"k", ""}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      " ==k=",
      { {"k", ""}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      " = =k=",
      { {"k", ""}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      " =k=",
      { {"k", ""}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      " =k=v",
      { {"k", "v"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      " ==k=v",
      { {"k", "v"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      " =k=v=w",
      { {"k", "v=w"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k=\xc3",
      { {"k", "\xc3"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k=\xc3v",
      { {"k", "\xc3v"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k=\xff",
      { {"k", "\xff"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k=\xffv",
      { {"k", "\xffv"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k=\"\xc3v",
      { {"k", "\xc3v"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k=\"\xff",
      { {"k", "\xff"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k=\"\xffv",
      { {"k", "\xffv"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "foo=",
      { {"foo", ""}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "foo=b",
      { {"foo", "b"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "lorem ipsum foo=bar",
      { {"foo", "bar"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "lorem ipsum/dolor @sitamen foo=bar",
      { {"foo", "bar"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "lorem ipsum/dolor = foo=bar",
      { {"foo", "bar"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "*k=v",
      { {"k", "v"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "x *k=v",
      { {"k", "v"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k1=v1 k2=v2 k3=v3",
      { {"k1", "v1"}, {"k2", "v2"}, {"k3", "v3"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k1=v1    k2=v2     k3=v3 ",
      { {"k1", "v1"}, {"k2", "v2"}, {"k3", "v3"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k1=v1,k2=v2,k3=v3",
      { {"k1", "v1,k2=v2,k3=v3"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k1=v1\tk2=v2 k3=v3",
      { {"k1", "v1\tk2=v2"}, {"k3", "v3"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k1=v1,\tk2=v2 k3=v3",
      { {"k1", "v1,\tk2=v2"}, {"k3", "v3"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k1=v1\t k2=v2 k3=v3",
      { {"k1", "v1\t"}, {"k2", "v2"}, {"k3", "v3"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k=\t",
      { {"k", "\t"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k=,\t",
      { {"k", ",\t"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "foo=\"bar\"",
      { {"foo", "bar"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k1=\"v1\", k2=\"v2\"",
      { {"k1", "v1"}, {"k2", "v2"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k1=\"\\\"v1\"",
      { {"k1", "\"v1"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k1=\"\\b \\f \\n \\r \\t \\\\\"",
      { {"k1", "\b \f \n \r \t \\"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k1=\"\\p\"",
      { {"k1", "\\p"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k1='\\'v1'",
      { {"k1", "'v1"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k1='\\b \\f \\n \\r \\t \\\\'",
      { {"k1", "\b \f \n \r \t \\"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k1='\\p'",
      { {"k1", "\\p"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k1=\\b\\f\\n\\r\\t\\\\",
      { {"k1", "\\b\\f\\n\\r\\t\\\\"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k1=\b\f\n\r\\",
      { {"k1", "\b\f\n\r\\"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k1=\"v foo, foo2 =@,\\\"\" k2='v foo,  a='",
      { {"k1", "v foo, foo2 =@,\""}, {"k2", "v foo,  a="}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k1=v1, k2=v2, k3=v3",
      { {"k1", "v1"}, {"k2", "v2"}, {"k3", "v3"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "foo=bar lorem ipsum key=value some more values",
      { {"foo", "bar"}, {"key", "value"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k1=v1,   k2=v2  ,    k3=v3",
      { {"k1", "v1"}, {"k2", "v2"}, {"k3", "v3"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k1 k2=v2, k3, k4=v4",
      { {"k2", "v2"}, {"k4", "v4"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k1= k2=v2, k3=, k4=v4 k5= , k6=v6",
      { {"k1", ""}, {"k2", "v2"}, {"k3", ""}, {"k4", "v4"}, {"k5", ""}, {"k6", "v6"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k1= v1 k2 = v2 k3 =v3 ",
      { {"k1", ""}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k1='v1', k2='v2'",
      { {"k1", "v1"}, {"k2", "v2"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      ", k=v",
      { {"k", "v"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      ",k=v",
      { {"k", "v"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k=v,",
      { {"k", "v,"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k=v, ",
      { {"k", "v"}, NULLKV }
    },
    {
      {':', FALSE},
      "k1:v1 k2:v2 k3:v3 ",
      { {"k1", "v1"}, {"k2", "v2"}, {"k3", "v3"}, NULLKV }
    },
    {
      {':', FALSE},
      "k1: v1 k2 : v2 k3 :v3 ",
      { {"k1", ""}, NULLKV }
    },
    {
      {'-', FALSE},
      "k-v",
      { {"k", "v"}, NULLKV }
    },
    {
      {'-', FALSE},
      "k--v",
      { {"k", "-v"}, NULLKV }
    },
    {
      {'-', FALSE},
      "---",
      { {"-", "-"}, NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "=v",
      { NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "k*=v",
      { NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "=",
      { NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "==",
      { NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "===",
      { NULLKV }
    },
    {
      DEFAULT_CONFIG,
      " =",
      { NULLKV }
    },
    {
      DEFAULT_CONFIG,
      " ==",
      { NULLKV }
    },
    {
      DEFAULT_CONFIG,
      " ===",
      { NULLKV }
    },
    {
      DEFAULT_CONFIG,
      " = =",
      { NULLKV }
    },
    {
      DEFAULT_CONFIG,
      ":=",
      { NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "á=v",
      { NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "",
      { NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "f",
      { NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "fo",
      { NULLKV }
    },
    {
      DEFAULT_CONFIG,
      "foo",
      { NULLKV }
    },
    {
      {'=', TRUE},
      "foo=bar",
      { {"foo", "bar"}, NULLKV }
    },
    {
      {'=', TRUE},
      "foo=bar",
      { {"foo", "bar"}, NULLKV }
    },
    {
      {'=', TRUE},
      "foo =bar",
      { {"foo", "bar"}, NULLKV }
    },
    {
      {'=', TRUE},
      "foo =bar",
      { {"foo", "bar"}, NULLKV }
    },
    {
      {'=', TRUE},
      "foo=bar ggg",
      { {"foo", "bar ggg"}, NULLKV }
    },
    {
      {'=', TRUE},
      "foo=bar ggg baz=ez",
      { {"foo", "bar ggg"}, {"baz", "ez"}, NULLKV }
    },
    {
      {'=', TRUE},
      " foo =bar ggg baz=ez",
      { {"foo", "bar ggg"}, {"baz", "ez"}, NULLKV }
    },
    {
      {'=', TRUE},
      "foo =bar ggg baz =ez",
      { {"foo", "bar ggg"}, {"baz", "ez"}, NULLKV }
    },
    {
      {'=', TRUE},
      "foo =bar ggg baz   =ez",
      { {"foo", "bar ggg"}, {"baz", "ez"}, NULLKV }
    },
    {
      {'=', TRUE},
      "foo =  bar ggg baz   =   ez",
      { {"foo", "bar ggg"}, {"baz", "ez"}, NULLKV }
    },
    {
      {'=', TRUE},
      "a b c=d",
      { {"c", "d"}, NULLKV }
    },
    {
      {'=', TRUE},
      " k= b",
      { {"k", "b"}, NULLKV }
    },
    {
      {'=', TRUE},
      "k= a=b c=d",
      { {"k", ""}, {"a", "b"}, {"c", "d"}, NULLKV }
    },
    {
      {'=', TRUE},
      "k=a=b c=d",
      { {"k", ""}, {"a", "b"}, {"c", "d"}, NULLKV }
    },
    {
      {'=', TRUE},
      "foo=a \"bar baz\" ",
      { {"foo", "a \"bar baz\""}, NULLKV }
    },
    {
      {'=', TRUE},
      "foo=a \"bar baz",
      { {"foo", "a \"bar baz"}, NULLKV }
    },
    {
      {'=', TRUE},
      "foo=a \"bar baz c=d",
      { {"foo", "a \"bar baz"}, {"c", "d"}, NULLKV }
    },
    {
      {'=', TRUE},
      "foo=a \"bar baz\"=f c=d a",
      { {"foo", "a \"bar baz\"=f"}, {"c", "d a"}, NULLKV }
    },
    {
      {'=', TRUE},
      "foo=\"bar baz\"",
      { {"foo", "bar baz"}, NULLKV }
    },
    {
      {'=', TRUE},
      "foo=\"bar\" baz c=d",
      { {"foo", "\"bar\" baz"}, {"c", "d"}, NULLKV }
    },
    {
      {'=', TRUE},
      "foo=bar\"",
      { {"foo", "bar\""}, NULLKV }
    },
    {
      {'=', TRUE},
      "k==",
      { {"k", "="}, NULLKV }
    },
    {
      {'=', TRUE},
      "k===",
      { {"k", "=="}, NULLKV }
    },
    {
      {'=', TRUE},
      "k===a",
      { {"k", "==a"}, NULLKV }
    },
    {
      {'=', TRUE},
      "k===  a",
      { {"k", "==  a"}, NULLKV }
    },
    {
      {'=', TRUE},
      "k===a=b",
      { {"k", "==a=b"}, NULLKV }
    },
    {
      {'=', TRUE},
      "a==b=",
      { {"a", "=b="}, NULLKV }
    },
    {
      {'=', TRUE},
      "a=,=b=a",
      { {"a", ",=b=a"}, NULLKV }
    },
    {
      {'=', TRUE},
      "a= =a",
      { {"a", "=a"}, NULLKV }
    },

    { {0, FALSE  }, NULL, { NULLKV } }
  };

  return cases_without_allow_pair_separator_in_value;
}

static GString*
_expected_to_string(KV* kvs)
{
  GString* result = g_string_new("");
  gboolean first = TRUE;
  while (kvs->key)
  {
    if (!first)
    {
      g_string_append_c(result, ' ');
    }
    first = FALSE;
    g_string_append_printf(result, "%s=%s", kvs->key, kvs->value);
    kvs++;
  }

  return result;
}

static void
_run_testcase(Testcase tc)
{
  GString* pretty_expected = _expected_to_string(tc.expected);
  testcase_begin("input:(%s), expected:(%s)", tc.input, pretty_expected->str);
  _scan_kv_pairs_scanner(create_kv_scanner(tc.config), tc.input, tc.expected);
  testcase_end();
  g_string_free(pretty_expected, TRUE);
}

static void
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
  _test_value_separator_clone();
  _run_testcases(provide_cases_without_allow_pair_separator_in_value());
  if (testutils_deinit())
    return 0;
  else
    return 1;
}
