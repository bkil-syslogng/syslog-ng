/*
 * Copyright (c) 2010-2014 Balabit
 * Copyright (c) 2010-2014 Balázs Scheidler <balazs.scheidler@balabit.com>
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

#include "testutils.h"
#include "test_parsers_e2e.h"

PatternDB *patterndb;
GPtrArray *messages;
gchar *filename;

gboolean fail = FALSE;
gboolean verbose = FALSE;

#define test_fail(fmt, args...) \
do {\
  printf(fmt, ##args); \
  fail = TRUE; \
} while (0);

#define test_msg(fmt, args...) \
do { \
  if (verbose) printf(fmt, ##args); \
} while (0);



const gchar *pdb_parser_skeleton_prefix = \
    "<?xml version='1.0' encoding='UTF-8'?>\
          <patterndb version='3' pub_date='2010-02-22'>\
            <ruleset name='test1_program' id='480de478-d4a6-4a7f-bea4-0c0245d361e1'>\
                <pattern>test</pattern>\
                    <rule id='1' class='test1' provider='my'>\
                        <patterns>\
                         <pattern>";

const gchar *pdb_parser_skeleton_postfix =  \
    "</pattern>\
                      </patterns>\
                    </rule>\
            </ruleset>\
        </patterndb>";


static void
test_pattern(const gchar *pattern, const gchar *rule, gboolean match)
{
  gboolean result;
  LogMessage *msg = log_msg_new_empty();

  log_msg_set_value(msg, LM_V_HOST, MYHOST, strlen(MYHOST));
  log_msg_set_value(msg, LM_V_PROGRAM, "test", strlen(MYHOST));
  log_msg_set_value(msg, LM_V_MESSAGE, pattern, strlen(pattern));

  result = pattern_db_process(patterndb, msg);

  if (match)
    {
      assert_true(result, "Value '%s' is not matching for pattern '%s'\n", rule, pattern);
    }
  else
    {
      assert_false(result, "Value '%s' is matching for pattern '%s'\n", rule, pattern);
    }

  log_msg_unref(msg);
}

void
_destroy_pattern_db(void)
{
  if (messages)
    {
      g_ptr_array_foreach(messages, (GFunc) log_msg_unref, NULL);
      g_ptr_array_free(messages, TRUE);
    }
  messages = NULL;
  pattern_db_free(patterndb);
  patterndb = NULL;

  g_unlink(filename);
  g_free(filename);
  filename = NULL;
}

static void
test_parser(const gchar **test)
{
  GString *str;
  gint index = 1;

  str = g_string_new(pdb_parser_skeleton_prefix);
  g_string_append(str, test[0]);
  g_string_append(str, pdb_parser_skeleton_postfix);

  _load_pattern_db_from_string(str->str);
  g_string_free(str, TRUE);
  while(test[index] != NULL)
    test_pattern(test[index++], test[0], TRUE);
  index++;
  while(test[index] != NULL)
    test_pattern(test[index++], test[0], FALSE);

  _destroy_pattern_db();
}

const gchar * test1 [] = {
"@ANYSTRING:TEST@",
"ab ba ab",
"ab ba ab",
"1234ab",
"ab1234",
"1.2.3.4",
"ab  1234  ba",
"&lt;ab ba&gt;",
NULL,
NULL
};

const gchar * test2 [] = {
"@DOUBLE:TEST@",
"1234",
"1234.567",
"1.2.3.4",
"1234ab",
NULL, // not match
"ab1234",NULL
};

const gchar * test3 [] = {
"@ESTRING:TEST:endmark@",
"ab ba endmark",
NULL,
"ab ba",NULL
};

const gchar * test4 [] = {
"@ESTRING:TEST:&gt;@",
"ab ba > ab",
NULL,
"ab ba",NULL
};

const gchar * test5 [] = {
"@FLOAT:TEST@",
"1234",
"1234.567",
"1.2.3.4",
"1234ab",
NULL, // not match
"ab1234",NULL
};

const gchar * test6 [] = {
"@SET:TEST: 	@",
" a ",
"  a ",
" 	a ",
" 	 a ",
NULL, // not match
"ab1234",NULL
};

const gchar * test7 [] = {
"@IPv4:TEST@",
"1.2.3.4",
"0.0.0.0",
"255.255.255.255",
NULL,
"256.256.256.256",
"1234",
"ab1234",
"ab1.2.3.4",
"1,2,3,4",NULL
};

const gchar * test8 [] = {
"@IPv6:TEST@",
"2001:0db8:0000:0000:0000:0000:1428:57ab",
"2001:0db8:0000:0000:0000::1428:57ab",
"2001:0db8:0:0:0:0:1428:57ab",
"2001:0db8:0:0::1428:57ab",
"2001:0db8::1428:57ab",
"2001:db8::1428:57ab",
NULL,
"2001:0db8::34d2::1428:57ab",NULL
};

const gchar * test9 [] = {
"@IPvANY:TEST@",
"1.2.3.4",
"0.0.0.0",
"255.255.255.255",
"2001:0db8:0000:0000:0000:0000:1428:57ab",
"2001:0db8:0000:0000:0000::1428:57ab",
"2001:0db8:0:0:0:0:1428:57ab",
"2001:0db8:0:0::1428:57ab",
"2001:0db8::1428:57ab",
"2001:db8::1428:57ab",
NULL,
"256.256.256.256",
"1234",
"ab1234",
"ab1.2.3.4",
"1,2,3,4",
"2001:0db8::34d2::1428:57ab",NULL
};

const gchar * test10 [] = {
"@NUMBER:TEST@",
"1234",
"1.2",
"1.2.3.4",
"1234ab",
NULL,
"ab1234",
NULL
};

const gchar * test11 [] = {
"@QSTRING:TEST:&lt;&gt;@",
"<aa bb>",
"< aabb >",
NULL,
"aabb>",
"<aabb",NULL
};

const gchar * test12 [] = {
"@STRING:TEST@",
"aabb",
"aa bb",
"1234",
"ab1234",
"1234bb",
"1.2.3.4",
NULL,NULL
};

const gchar **parsers[] = {test1, test2, test3, test4, test5, test6, test7, test8, test9, test10, test11, test12, NULL};

void
test_patterndb_parsers(void)
{
  gint i;

  for (i = 0; parsers[i]; i++)
    {
      test_parser(parsers[i]);
    }
}

static void
_emit_func(LogMessage *msg, gboolean synthetic, gpointer user_data)
{
  g_ptr_array_add(messages, log_msg_ref(msg));
}

void
_load_pattern_db_from_string(const gchar *pdb)
{
  patterndb = pattern_db_new();
  messages = g_ptr_array_new();

  pattern_db_set_emit_func(patterndb, _emit_func, NULL);

  g_file_open_tmp("patterndbXXXXXX.xml", &filename, NULL);
  g_file_set_contents(filename, pdb, strlen(pdb), NULL);

  assert_true(pattern_db_reload_ruleset(patterndb, configuration, filename), "Error loading ruleset [[[%s]]]", pdb);
  assert_string(pattern_db_get_ruleset_version(patterndb), "3", "Invalid version");
  assert_string(pattern_db_get_ruleset_pub_date(patterndb), "2010-02-22", "Invalid pubdate");
}
