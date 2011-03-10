include $(top_srcdir)/aminclude_static.am

lib_LTLIBRARIES = libpyplug.la

INCLUDES = 	$(DEPS_CFLAGS) \
	   	$(PYTHON_CPPFLAGS)

LIBS = 		$(DEPS_LIBS) \
		$(PYTHON_LIBS)

AM_LDFLAGS = 	-no-undefined \
		$(PYTHON_LDFLAGS)

libpyplug_la_SOURCES = pyplug.c

SPLINTFLAGS = $(INCLUDES) -castfcnptr
