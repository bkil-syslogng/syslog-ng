moduledir = @moduledir@
export top_srcdir

SUBDIRS = . tests

AM_CPPFLAGS	= \
  -I$(top_srcdir)/lib \
  -I../../lib \
  -I$(top_srcdir)/modules/kvformat \
  -I$(top_builddir)/modules/kvformat
module_LTLIBRARIES = libkvformat.la

libkvformat_la_SOURCES = \
  format-welf.c \
  format-welf.h \
  kv-parser.c \
  kv-parser.h \
  kv-scanner.c \
  kv-scanner.h \
  kv-scanner-generic.c \
  kv-scanner-generic.h \
  linux-audit-scanner.c \
  linux-audit-scanner.h \
  kv-parser-grammar.y \
  kv-parser-parser.c \
  kv-parser-parser.h \
  kvformat-plugin.c

libkvformat_la_CPPFLAGS = $(AM_CPPFLAGS)
libkvformat_la_LIBADD = $(MODULE_DEPS_LIBS)
libkvformat_la_LDFLAGS = $(MODULE_LDFLAGS)

BUILT_SOURCES = \
  kv-parser-grammar.y \
  kv-parser-grammar.c \
  kv-parser-grammar.h
EXTRA_DIST = $(BUILT_SOURCES) kv-parser-grammar.ym

include $(top_srcdir)/build/lex-rules.am
