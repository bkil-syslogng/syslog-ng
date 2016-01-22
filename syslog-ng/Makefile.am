sbin_PROGRAMS			+= syslog-ng/syslog-ng

syslog_ng_syslog_ng_SOURCES	= syslog-ng/main.c

# NOTE: we're referring to the syslog-ng lib directly instead of via
# libtool, because in mixed mode libtool is not used for the final linking
# phase.  See the comment in the configure script for details.

syslog_ng_syslog_ng_LDADD		= -L${top_builddir}/lib/.libs -lsyslog-ng @SYSLOGNG_DEPS_LIBS@
syslog_ng_syslog_ng_LINK		=  @SYSLOGNG_LINK@
syslog_ng_syslog_ng_DEPENDENCIES	= lib/libsyslog-ng.la

syslog-ng syslog-ng/: ${sbin_PROGRAMS}
.PHONY: syslog-ng/
