/*
 * Copyright (c) 2002-2016 Balabit
 * Copyright (c) 2016 Viktor Juhasz <viktor.juhasz@balabit.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */

#include "diskq.h"
#include "cfg-parser.h"
#include "diskq-grammar.h"

extern int diskq_debug;
int diskq_parse(CfgLexer *lexer, LogDriverPlugin **instance, gpointer arg);

static CfgLexerKeyword diskq_keywords[] = {
  { "disk_buffer",       KW_DISK_BUFFER },
  { "mem_buf_length",    KW_MEM_BUF_LENGTH },
  { "disk_buf_size",     KW_DISK_BUF_SIZE },
  { "reliable",          KW_RELIABLE },
  { "mem_buf_size",      KW_MEM_BUF_SIZE },
  { "qout_size",         KW_QOUT_SIZE },
  { "dir",               KW_DIR },
  { NULL }
};

CfgParser diskq_parser =
{
#if ENABLE_DEBUG
  .debug_flag = &diskq_debug,
#endif
  .name = "disk_buffer",
  .keywords = diskq_keywords,
  .parse = (int (*)(CfgLexer *lexer, gpointer *instance, gpointer arg)) diskq_parse,
  .cleanup = (void (*)(gpointer)) log_pipe_unref,

};

CFG_PARSER_IMPLEMENT_LEXER_BINDING(diskq_, LogDriverPlugin **)
