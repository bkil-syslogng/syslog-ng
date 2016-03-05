
LIBMONGO_CFLAGS += -Werror -W -Wall -Wextra -Wshadow \
 -fno-common -Wdouble-promotion -Wformat=2 -Wcast-align \
 -Wbad-function-cast -Wmissing-prototypes -Wmissing-declarations \
 -Wnested-externs -Wcast-qual -Wwrite-strings -Wfloat-equal \
 -Wabi -Waggregate-return -Winvalid-pch \
 -Wmissing-format-attribute -Wdeprecated-declarations -Woverflow \
 -Wpointer-arith -Wpointer-sign \
 -Wno-unused-parameter -Wno-missing-field-initializers \
 -Wimplicit-function-declaration \
 -Wstrict-prototypes -Winline -Wstack-protector \
 -Wold-style-definition

EXTRA_FLAGS = -Wstrict-overflow=2 -Wconversion -Wsign-conversion \
 -Wvariadic-macros -Wswitch-default \
 -Wredundant-decls -Wlong-long
# -Wpedantic -Wundef

DIST_SUBDIRS 					+= modules/afmongodb/mongo-c-driver

if LIBMONGO_INTERNAL
CLEAN_SUBDIRS					+= @LIBMONGO_SUBDIRS@
lmc_EXTRA_DEPS					= modules/afmongodb/mongo-c-driver/libmongoc-1.0.la

modules/afmongodb/mongo-c-driver/libmongoc-1.0.la:
	${MAKE} -C modules/afmongodb/mongo-c-driver
endif

if ENABLE_MONGODB

module_LTLIBRARIES 				+= modules/afmongodb/libafmongodb.la

modules_afmongodb_libafmongodb_la_CFLAGS 	=	\
	$(LIBMONGO_CFLAGS) 				\
	-I$(top_srcdir)/modules/afmongodb		\
	-I$(top_builddir)/modules/afmongodb
modules_afmongodb_libafmongodb_la_SOURCES	=	\
	modules/afmongodb/afmongodb-grammar.y		\
	modules/afmongodb/afmongodb.c			\
	modules/afmongodb/afmongodb.h			\
	modules/afmongodb/afmongodb-private.h			\
	modules/afmongodb/afmongodb-parser.c		\
	modules/afmongodb/afmongodb-parser.h		\
	${DUMMY_C}

if ENABLE_LEGACY_MONGODB_OPTIONS
modules_afmongodb_libafmongodb_la_SOURCES	+=	\
	modules/afmongodb/afmongodb-legacy-uri.c			\
	modules/afmongodb/afmongodb-legacy-uri.h			\
	modules/afmongodb/afmongodb-legacy-grammar.c			\
	modules/afmongodb/afmongodb-legacy-grammar.h			\
	modules/afmongodb/afmongodb-legacy-private.h
endif

modules_afmongodb_libafmongodb_la_LIBADD	=	\
	$(MODULE_DEPS_LIBS) $(LIBMONGO_LIBS)
modules_afmongodb_libafmongodb_la_LDFLAGS	=	\
	$(MODULE_LDFLAGS)
modules_afmongodb_libafmongodb_la_DEPENDENCIES	=	\
	$(MODULE_DEPS_LIBS) ${lmc_EXTRA_DEPS}

modules/afmongodb modules/afmongodb/ mod-afmongodb mod-mongodb: \
	modules/afmongodb/libafmongodb.la

include modules/afmongodb/tests/Makefile.am
else
modules/afmongodb modules/afmongodb/ mod-afmongodb mod-mongodb:
endif

modules/afmongodb/dummy.c: ${lmc_EXTRA_DEPS}
	$(AM_V_GEN) touch $@

BUILT_SOURCES					+=	\
	modules/afmongodb/afmongodb-grammar.y		\
	modules/afmongodb/afmongodb-grammar.c		\
	modules/afmongodb/afmongodb-grammar.h		\
	modules/afmongodb/dummy.c
EXTRA_DIST					+=	\
	modules/afmongodb/afmongodb-grammar.ym		\
	modules/afmongodb/mongo-c-driver/configure.gnu

.PHONY: modules/afmongodb/ mod-afmongodb mod-mongodb
