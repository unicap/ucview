#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
dist_version=0.1

AC_PREREQ(2.61)
AC_INIT(ucview_debayer_plugin, 0.1)
AC_SUBST(dist_version)
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.
PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.8.0)
PKG_CHECK_MODULES(UNICAP, [ucview libunicap libucil libglade-2.0 gtk+-2.0 gconf-2.0])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset])



AC_OUTPUT([
Makefile 
src/Makefile
])
