module_LTLIBRARIES				+= modules/kvtagger/libkvtagger.la
modules_kvtagger_libkvtagger_la_SOURCES	=	\
	modules/kvtagger/kvtagger.c			\
	modules/kvtagger/kvtagger.h			\
	modules/kvtagger/csv-tagrecord-scanner.c	\
	modules/kvtagger/csv-tagrecord-scanner.h	\
	modules/kvtagger/tagrecord-scanner.h		\
	modules/kvtagger/tagrecord-scanner.c		\
	modules/kvtagger/kvtagger-grammar.y		\
	modules/kvtagger/kvtagger-parser.c		\
	modules/kvtagger/kvtagger-parser.h		\
	modules/kvtagger/kvtagdb.h			\
	modules/kvtagger/kvtagdb.c			\
	modules/kvtagger/kvtagger-plugin.c

EXTRA_WARNINGS = \
 -std=gnu11 \
 -W -Wall -Wextra -Wshadow -fno-common -Wdouble-promotion -Wformat=2 \
 -Wcast-align -Wbad-function-cast -Wmissing-prototypes -Wmissing-declarations -Winline \
 -Wnested-externs -Wcast-qual -Wwrite-strings -Wfloat-equal -Wabi -Wno-stack-protector \
 -Waggregate-return -Winvalid-pch -Wmissing-format-attribute -Wdeprecated \
 -Wdeprecated-declarations -Woverflow -Wunused-parameter \
 -Wvariadic-macros -Wpointer-arith -Wimplicit-function-declaration \
 -Wlong-long -Wstrict-overflow=2 -Wpointer-sign \
 \
 -Wno-error=sign-conversion \
 -Wno-error=conversion \
 -Wno-error=undef \
 -Wno-error=unused-parameter \
 -Wno-error=switch-default \
 -Wno-error=missing-field-initializers \
 -Wno-error=redundant-decls \
 -Wno-error=strict-prototypes \
 -Wno-error=strict-overflow

SOMETHING = \
 -Wpedantic

modules_kvtagger_libkvtagger_la_CPPFLAGS	=	\
	$(AM_CPPFLAGS)					\
	-I$(top_srcdir)/modules/kvtagger		\
	-I$(top_builddir)/modules/kvtagger \
    $(EXTRA_WARNINGS)
modules_kvtagger_libkvtagger_la_LIBADD	=	\
	$(MODULE_DEPS_LIBS)
modules_kvtagger_libkvtagger_la_LDFLAGS	=	\
	$(MODULE_LDFLAGS)
modules_kvtagger_libkvtagger_la_DEPENDENCIES	=	\
	$(MODULE_DEPS_LIBS)

BUILT_SOURCES					+=	\
	modules/kvtagger/kvtagger-grammar.y		\
	modules/kvtagger/kvtagger-grammar.c		\
	modules/kvtagger/kvtagger-grammar.h
EXTRA_DIST					+=	\
	modules/kvtagger/kvtagger-grammar.ym

modules/kvtagger modules/kvtagger/ mod-kvtagger: modules/kvtagger/libkvtagger.la
.PHONY: modules/kvtagger/ mod-kvtagger

include modules/kvtagger/tests/Makefile.am
