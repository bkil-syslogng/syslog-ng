bin_PROGRAMS			+= tests/loggen/loggen

tests_loggen_loggen_LDADD	= @BASE_LIBS@ @OPENSSL_LIBS@ @GLIB_LIBS@
tests_loggen_loggen_CFLAGS = \
	$(AM_CFLAGS)	\
	-Wno-error=sign-compare -Wno-error=cast-qual
