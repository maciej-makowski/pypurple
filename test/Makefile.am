noinst_LTLIBRARIES = test_pyplug.la test_pyplug_python.la

INCLUDES = 	$(CUTTER_CFLAGS) \
		$(DEPS_CFLAGS) \
		$(PYTHON_CPPFLAGS) \
		-I$(top_srcdir)/src/pyplug

LIBS = $(CUTTER_LIBS) \
       $(DEPS_LIBS) \
       $(PYTHON_LIBS) \
       $(top_builddir)/src/pyplug/libpyplug.la

AM_LDFLAGS = -module -rpath $(libdir) -avoid-version -no-undefined $(PYTHON_LDFLAGS)

TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE=yes CUTTER="$(CUTTER)"

test_pyplug_la_SOURCES = test-pyplug.c
test_pyplug_python_la_SOURCES = test-pyplug-python.c

echo-cutter:
	echo $(CUTTER)
