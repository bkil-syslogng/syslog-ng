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

#ifndef TEST_PARSERS_E2E_H_
#define TEST_PARSERS_E2E_H_

#include "apphook.h"
#include "tags.h"
#include "logmsg/logmsg.h"
#include "messages.h"
#include "filter/filter-expr.h"
#include "plugin.h"
#include "cfg.h"
#include "timerwheel.h"
#include "libtest/msg_parse_lib.h"

#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <glib/gstdio.h>
#include "patterndb.h"

#define MYHOST "MYHOST"

extern PatternDB *patterndb;
extern GPtrArray *messages;
extern gchar *filename;

void test_patterndb_parsers(void);
void _load_pattern_db_from_string(const gchar *pdb);
void _destroy_pattern_db(void);

#endif
