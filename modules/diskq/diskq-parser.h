/*
 * Copyright (C) 2006-2011 BalaBit IT Ltd.
 *
 * All rights reserved.
 */
#ifndef DISKQ_PARSER_H_INCLUDED
#define DISKQ_PARSER_H_INCLUDED

#include "cfg-parser.h"
#include "cfg-lexer.h"
#include "diskq.h"

extern CfgParser diskq_parser;

CFG_PARSER_DECLARE_LEXER_BINDING(diskq_, LogDriverPlugin **)

#endif
