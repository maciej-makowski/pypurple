#-######################################################################
## LIBPURPLE_DEVEL.m4 - Autoconf macro searching for libpurple
##	library and header files.
##
## Copyright (c) 2011 by Maciej Makowski
##
## This file is free software; you can redistribute it and/or modify it
## under the terms of the GNU Lesser General Public License as published
## by the Free Software Foundation; either version 2.1 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## Lesser General Public License for more details.
##
##-######################################################################

dnl @synopsis LIBPURPLE_DEVEL
dnl 
dnl This macro tries to find the libpurple library and header files.
dnl
dnl We define the following configure script flags:
dnl
dnl     --with-purple: 	Give prefix for both library and headers, and try to 
dnl     		guess subdirectory names for each.  (e.g. tack /lib 
dnl     		and /include onto given dir name, and other common 
dnl     		schemes.)
dnl
dnl	--with-purple-lib: Similar to --with-purple, but for library only.
dnl		
dnl	--with-purple-include: Similar to --with-purple, but for headers only.
dnl
dnl @version 0.1, 2011/03/16
dnl @author Maciej Makowski <mmakowski@cfiet.net>

AC_DEFUN([LIBPURPLE_DEVEL],
[
	AC_ARG_WITH(purple,
	[  --with-purple=<path>		path containing libpurple header and library subdirs and files],
	[LIBPURPLE_lib_check="$with_purple/lib $with_purple/lib/purple $with_purple/lib/libpurple $with_purple/purple/.libs $with_purple/libpurple/.libs" 
	 LIBPURPLE_inc_check="$with_purple/include $with_purple/include/purple $with_purple/include/libpurple $with_purple/libpurple" ],
	[LIBPURPLE_lib_check="/usr/local/lib /usr/local/lib/purple /usr/local/lib/libpurple /usr/lib /usr/lib/purple /usr/lib/libpurple"
	 LIBPURPLE_inc_check="/usr/local/include /usr/local/include/purple /usr/local/include/libpurple /usr/include /usr/include/purple /usr/include/libpurple"
	])
	AC_ARG_WITH(purple-lib,
	[  --with-purple-lib=<path>	path containing libpurple library subdirs and files],
	[LIBPURPLE_lib_check="$with_purple_lib/lib $with_purple_lib/lib/purple $with_purple_lib/lib/libpurple $with_purple_lib/purple/.libs $with_purple_lib/libpurple/.libs"])
	AC_ARG_WITH(purple-include,
	[  --with-purple-include=<path>	path containing libpurple header subdirs and files],
	[LIBPURPLE_inc_check="$with_purple_include/include $with_purple_include/include/purple $with_purple_include/include/libpurple $with_purple_include/libpurple"])

	# 
	# Find path to the header files
	#
	AC_MSG_CHECKING([for purple header files])
	PURPLE_INCLUDE=""
	for dir in $LIBPURPLE_inc_check
	do
		if test -d "$dir" && test -f "$dir/purple.h"
		then
			PURPLE_INCLUDE=$dir
			break
		fi
	done

	if test -z "$PURPLE_INCLUDE"
	then
		AC_MSG_ERROR([Cannot find purple headers in $LIBPURPLE_inc_check])
	fi

	case "$PURPLE_INCLUDE" in
		/* ) ;;
		* )  AC_MSG_ERROR([The purple headers directory path ($PURPLE_INCLUDE) must be absolute.]) ;;
	esac
	AC_MSG_RESULT([found in $PURPLE_INCLUDE])
	PURPLE_CFLAGS+=-I$PURPLE_INCLUDE
	AC_SUBST([PURPLE_CFLAGS])

	#
	# Find path to library
	#
	AC_MSG_CHECKING([for purple library])
	PURPLE_LIB=""
	for dir in $LIBPURPLE_lib_check
	do
		if test -d "$dir" && test -f "$dir/libpurple.so"
		then
			PURPLE_LIB=$dir
			break
		fi
	done
	
	if test -z "$PURPLE_LIB"
	then
		AC_MSG_ERROR([Cannot find purple in $LIBPURPLE_lib_check])
	fi

	case "$PURPLE_LIB" in
		/* ) ;;
		* )  AC_MSG_ERROR([The purple library directory path ($PURPLE_LIB) must be absolute.]) ;;
	esac
	AC_MSG_RESULT([found in $PURPLE_LIB])
	PURPLE_LIBS+="-L$PURPLE_LIB -lpurple"
	AC_SUBST([PURPLE_LIBS])

])

