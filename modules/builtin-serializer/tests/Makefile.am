modules_builtin_serializer_tests_TESTS      = modules/builtin-serializer/tests/test_builtin_serializer \
                                              modules/builtin-serializer/tests/test_gsockaddr_serialize \
                                              modules/builtin-serializer/tests/test_timestamp_serialize
check_PROGRAMS          += ${modules_builtin_serializer_tests_TESTS}

modules_builtin_serializer_tests_test_builtin_serializer_CFLAGS = $(TEST_CFLAGS) -I$(top_srcdir)/modules/builtin-serializer
modules_builtin_serializer_tests_test_builtin_serializer_LDADD  = \
  $(TEST_LDADD)         \
  $(PREOPEN_SYSLOGFORMAT) $(PREOPEN_BUILTIN_SERIALIZER)

modules_builtin_serializer_tests_test_gsockaddr_serialize_CFLAGS = $(TEST_CFLAGS) -I$(top_srcdir)/modules/builtin-serializer
modules_builtin_serializer_tests_test_gsockaddr_serialize_LDADD  = \
  $(TEST_LDADD)         \
  $(PREOPEN_SYSLOGFORMAT) $(PREOPEN_BUILTIN_SERIALIZER)

modules_builtin_serializer_tests_test_timestamp_serialize_CFLAGS = $(TEST_CFLAGS) -I$(top_srcdir)/modules/builtin-serializer
modules_builtin_serializer_tests_test_timestamp_serialize_LDADD  = \
  $(TEST_LDADD)         \
  $(PREOPEN_SYSLOGFORMAT) $(PREOPEN_BUILTIN_SERIALIZER)
