TEST_LIBMONGO_CFLAGS = -Werror -W -Wall -Wextra -Wshadow \
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
 -Wredundant-decls -Wlong-long \
 -Wno-error=strict-prototypes -Wno-error=sign-conversion -Wno-error=conversion \
 -Wno-error=cast-qual
# -Wpedantic -Wundef

modules_afmongodb_tests_TESTS          = \
       modules/afmongodb/tests/test-mongodb-config

check_PROGRAMS                         += ${modules_afmongodb_tests_TESTS}

modules_afmongodb_tests_test_mongodb_config_CFLAGS = \
    $(LIBMONGO_CFLAGS) \
    $(TEST_CFLAGS) \
    $(TEST_LIBMONGO_CFLAGS) \
    $(EXTRA_FLAGS)

modules_afmongodb_tests_test_mongodb_config_LDADD        = \
    $(TEST_LDADD) \
    -dlpreopen $(top_builddir)/modules/afmongodb/libafmongodb.la \
    ${lmc_EXTRA_DEPS}
