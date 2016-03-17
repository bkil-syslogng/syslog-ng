bin_PROGRAMS				+= modules/dbparser/pdbtool/pdbtool

modules_dbparser_pdbtool_pdbtool_SOURCES =	\
	modules/dbparser/pdbtool/pdbtool.c
modules_dbparser_pdbtool_pdbtool_CFLAGS =	\
	$(AM_CFLAGS)				\
	@CFLAGS_NOWARN_POINTER_SIGN@ \
	-Wno-error=pointer-sign -Wno-error=cast-qual -Wno-error=sign-compare
modules_dbparser_pdbtool_pdbtool_CPPFLAGS=	\
	$(AM_CPPFLAGS)				\
	-I$(top_srcdir)/modules/dbparser	\
	-I$(top_builddir)/modules/dbparser
modules_dbparser_pdbtool_pdbtool_LDADD	=	\
	$(top_builddir)/lib/libsyslog-ng.la	\
	$(top_builddir)/modules/dbparser/libsyslog-ng-patterndb.la \
	@TOOL_DEPS_LIBS@
