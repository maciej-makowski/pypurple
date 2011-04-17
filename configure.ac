AC_PREREQ(2.67)

m4_define([pyprpl_name], [pyprpl])
m4_define([pyprpl_version_major], [0])
m4_define([pyprpl_version_minor], [1])
m4_define([pyprpl_version_suffix], [a])
m4_define([pyprpl_version], pyprpl_version_major.pyprpl_version_minor)
m4_define([pyprpl_version_display],
		  pyprpl_version[]m4_ifdef([pyprpl_version_suffix],[pyprpl_version_suffix]))

m4_define([pyprpl_author], [Maciej Makowski])
m4_define([pyprpl_author_mail], [cfiet@cfiet.net])
m4_define([pyprpl_author_homepage], [http://www.cfiet.net])

m4_define([purple_plugin_id], [lopl-cfiet-pyprpl])
m4_define([purple_plugin_name], [Python plugin loader])
m4_define([purple_plugin_version], pyprpl_version_display)
m4_define([purple_plugin_author], pyprpl_author <[]pyprpl_author_mail>)
m4_define([purple_plugin_homepage], pyprpl_author_homepage)

AC_INIT(pyprpl_name, pyprpl_version_display, pyprpl_author_mail)

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_SRCDIR([src/pyplug.c])
AC_CONFIG_HEADER([src/config.h])

AM_INIT_AUTOMAKE($PACKAGE_NAME, $PACKAGE_VERSION)

AC_PROG_CC
AC_PROG_LIBTOOL

AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)
GLIB_MOD="glib-2.0 >= 2.14.0"
PKG_CHECK_MODULES(DEPS, $GLIB_MOD)

LIBPURPLE_DEVEL
AC_DEFINE_UNQUOTED([PYTHON_SONAME], "$ac_python_soname", [Linked python version])

AC_DEFINE_UNQUOTED(	[PYPRPL_PLUGIN_ID], "purple_plugin_id",
					[Plugin id for libpurple])
AC_DEFINE_UNQUOTED(	[PYPRPL_PLUGIN_VERSION], "purple_plugin_version",
					[Plugin version for libpurple])
AC_DEFINE_UNQUOTED(	[PYPRPL_PLUGIN_AUTHOR], "purple_plugin_author",
					[Plugin author for libpurple])
AC_DEFINE_UNQUOTED(	[PYPRPL_PLUGIN_HOMEPAGE], "purple_plugin_homepage",
					[Plugin homepage for libpurple])

PYTHON_MOD=">= 2.6"
AC_PYTHON_DEVEL($PYTHON_MOD)

AC_DEFINE_UNQUOTED([PYTHON_SONAME], "$ac_python_soname", [Linked python version])

AC_CHECK_CUTTER
#AX_PROG_SPLINT

AC_CONFIG_FILES([Makefile
		 src/Makefile
		 test/Makefile])
AC_OUTPUT
