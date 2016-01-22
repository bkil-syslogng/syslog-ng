module_LTLIBRARIES				+= modules/builtin-serializer/libbuiltin-serializer.la

modules_builtin_serializer_libbuiltin_serializer_la_SOURCES	=	\
  modules/builtin-serializer/builtin-serializer-plugin.c \
  modules/builtin-serializer/builtin-serializer.c \
  modules/builtin-serializer/builtin-serializer.h \
  modules/builtin-serializer/gsockaddr-serialize.h \
  modules/builtin-serializer/gsockaddr-serialize.c \
  modules/builtin-serializer/nvtable-serialize.h \
  modules/builtin-serializer/nvtable-serialize.c \
  modules/builtin-serializer/nvtable-serialize-endianutils.h \
  modules/builtin-serializer/sdata-serialize.h \
  modules/builtin-serializer/sdata-serialize.c \
  modules/builtin-serializer/tags-serialize.c \
  modules/builtin-serializer/tags-serialize.h \
  modules/builtin-serializer/timestamp-serialize.c \
  modules/builtin-serializer/timestamp-serialize.h

modules_builtin_serializer_libbuiltin_serializer_la_CPPFLAGS	=	\
	$(AM_CPPFLAGS)					\
	-I$(top_srcdir)/modules/builtin-serializer
modules_builtin_serializer_libbuiltin_serializer_la_LIBADD	=	\
	$(MODULE_DEPS_LIBS)
modules_builtin_serializer_libbuiltin_serializer_la_LDFLAGS	=	\
	$(MODULE_LDFLAGS)
modules_builtin_serializer_libbuiltin_serializer_la_DEPENDENCIES=	\
	$(MODULE_DEPS_LIBS)

modules/builtin-serializer modules/builtin-serializer/ mod-builtin-serializer: modules/builtin-serializer/libbuiltin-serializer.la
.PHONY: modules/builtin-serializer/ mod-builtin-serializer

include modules/builtin-serializer/tests/Makefile.am
