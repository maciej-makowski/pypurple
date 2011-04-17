include $(top_srcdir)/aminclude_static.am

lib_LTLIBRARIES = libpyplug.la liblopl-cfiet-pypurple.la

INCLUDES = 	$(DEPS_CFLAGS) \
			$(PYTHON_CPPFLAGS)

LIBS = 		$(DEPS_LIBS) \
			$(PYTHON_LIBS)

AM_LDFLAGS = 	--as-needed \
				$(PYTHON_LDFLAGS)

# libpyplug options
libpyplug_la_SOURCES = pyplug.c

# pidgin plugin options
liblopl_cfiet_pypurple_la_CPPFLAGS = 	$(INCLUDES) \
										$(PURPLE_CFLAGS)
liblopl_cfiet_pyurple_la_LDFLAGS = 		$(AM_LDFLAGS) \
										$(LIBS) \
										$(PURPLE_LIBS)
liblopl_cfiet_pypurple_la_DEPENDENCIES = libpyplug.la
liblopl_cfiet_pypurple_la_SOURCES = lopl-cfiet-pypurple.c pyplug.c pyprpl-module.c pyprpl-util.c
