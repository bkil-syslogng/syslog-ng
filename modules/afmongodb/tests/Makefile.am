modules_afmongodb_tests_TESTS		= \
	modules/afmongodb/tests/test-backwards-config-uri

check_PROGRAMS				+= ${modules_afmongodb_tests_TESTS}

modules_afmongodb_tests_test_backwards_config_uri_CFLAGS = \
    $(TEST_CFLAGS) \
    $(LIBMONGO_CFLAGS) \
    $(EXTRA_FLAGS)

modules_afmongodb_tests_test_backwards_config_uri_LDADD	= \
    $(TEST_LDADD) \
    -dlpreopen $(top_builddir)/modules/afmongodb/libafmongodb.la \
    ${lmc_EXTRA_DEPS}
