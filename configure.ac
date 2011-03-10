AC_PREREQ(2.67)
AC_INIT(pyplug, 0.1a, cfiet@cfiet.net)

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_SRCDIR([src/pyplug/pyplug.c])
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

AC_CHECK_LIB([purple], [purple_debug_info], ,AC_MSG_ERROR([No libpurple found.]))

PYTHON_MOD=">= 2.7"
AC_PYTHON_DEVEL($PYTHON_MOD)

AC_DEFINE_UNQUOTED([PYTHON_SONAME], "$ac_python_soname", [Linked python version])

AC_CHECK_CUTTER
AX_PROG_SPLINT([-weak])

AC_CONFIG_FILES([Makefile
		 src/Makefile
		 src/pyplug/Makefile
		 test/Makefile])
AC_OUTPUT
