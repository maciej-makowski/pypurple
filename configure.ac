AC_PREREQ(2.67)

m4_define([pyplug_name], [pyplug])
m4_define([pyplug_version_major], [0])
m4_define([pyplug_version_minor], [1])
m4_define([pyplug_version_suffix], [a])
m4_define([pyplug_version], pyplug_version_major.pyplug_version_minor)
m4_define([pyplug_version_display],
		  pyplug_version[]m4_ifdef([pyplug_version_suffix],[pyplug_version_suffix]))

m4_define([pyplug_author], [Maciej Makowski])
m4_define([pyplug_author_mail], [cfiet@cfiet.net])
m4_define([pyplug_author_homepage], [http://www.cfiet.net])

m4_define([purple_plugin_id], [lopl-cfiet-python])
m4_define([purple_plugin_name], [Python plugin loader])
m4_define([purple_plugin_version], pyplug_version_display)
m4_define([purple_plugin_author], pyplug_author <[]pyplug_author_mail>)
m4_define([purple_plugin_homepage], pyplug_author_homepage)

AC_INIT(pyplug_name, pyplug_version_display, pyplug_author_mail)

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_SRCDIR([src/pyplug.c])
AC_CONFIG_HEADER([src/config.h])

AM_INIT_AUTOMAKE($PACKAGE_NAME, $PACKAGE_VERSION)

m4_include([m4/AX_PYTHON30_DEVEL.m4])

AC_PROG_CC
AC_PROG_LIBTOOL

AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)
GLIB_MOD="glib-2.0 >= 2.14.0"
PKG_CHECK_MODULES(DEPS, $GLIB_MOD)

LIBPURPLE_DEVEL
AC_DEFINE_UNQUOTED([PYTHON_SONAME], "$ac_python_soname", [Linked python version])

AC_DEFINE_UNQUOTED(	[PYPLUG_PLUGIN_ID], "purple_plugin_id",
					[Plugin id for libpurple])
AC_DEFINE_UNQUOTED(	[PYPLUG_PLUGIN_VERSION], "purple_plugin_version",
					[Plugin version for libpurple])
AC_DEFINE_UNQUOTED(	[PYPLUG_PLUGIN_AUTHOR], "purple_plugin_author",
					[Plugin author for libpurple])
AC_DEFINE_UNQUOTED(	[PYPLUG_PLUGIN_HOMEPAGE], "purple_plugin_homepage",
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
