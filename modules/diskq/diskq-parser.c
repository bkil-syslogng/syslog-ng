/*
 * Copyright (C) 2006-2011 BalaBit IT Ltd.
 *
 * All rights reserved.
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
