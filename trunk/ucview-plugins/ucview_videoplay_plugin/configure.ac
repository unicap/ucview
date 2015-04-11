#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
dist_version=0.1

AC_PREREQ(2.61)
AC_INIT(ucview_videoplay_plugin, 0.1)
AC_SUBST(dist_version)
AM_INIT_AUTOMAKE


AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL

# Checks for libraries.
PKG_CHECK_MODULES(MODULES, [ ucview libunicap libucil libglade-2.0 glib-2.0 >= 2.8.0 gtk+-2.0 gconf-2.0 gstreamer-0.10 gstreamer-interfaces-0.10 ])

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
