export top_srcdir
export top_builddir
dbd_dir = $(shell echo $(exec_prefix)/lib/dbd)
export dbd_dir
FUNC_SRCDIR = $(top_srcdir)/tests/functional

EXTRA_DIST	+= \
		tests/functional/runtests.sh \
		tests/functional/func_test.py \
		tests/functional/control.py \
		tests/functional/globals.py \
		tests/functional/log.py \
		tests/functional/messagecheck.py \
		tests/functional/messagegen.py \
		tests/functional/ssl.crt tests/functional/ssl.key tests/functional/rnd.in \
		tests/functional/test_file_source.py \
		tests/functional/test_filters.py \
		tests/functional/test_input_drivers.py \
		tests/functional/test_performance.py \
		tests/functional/test_python.py \
		tests/functional/test_sql.py

func-test:
	@$(top_srcdir)/tests/functional/runtests.sh

func-py-checks: func-py-pep8 func-py-pylint

func-py-pep8:
	pep8 --ignore=E501 $(FUNC_SRCDIR)/*.py

func-py-pylint:
	pylint -r n --rcfile=$(FUNC_SRCDIR)/pylintrc $(FUNC_SRCDIR)/*.py

.PHONY: func-test

tests/functional/rnd: tests/functional/rnd.in
	cp $< $@

CLEANFILES	+= 					\
		tests/functional/test.conf		\
		tests/functional/rnd			\
		tests/functional/syslog-ng.persist	\
		tests/functional/test-performance.log
