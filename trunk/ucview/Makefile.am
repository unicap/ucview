## Process this file with automake to produce Makefile.in
PACKAGE=@PACKAGE@
VERSION=@VERSION@

SUBDIRS = \
	src \
	desktop \
	data \
	icons \
	plugins \
	po

dist_filename = @PACKAGE@-@VERSION@.tar.gz

EXTRA_DIST = \
	ChangeLog \
	intltool-extract.in \
	intltool-merge.in 	\
	intltool-update.in \
	config.rpath 

DISTCLEANFILES = 		\
	intltool-extract 	\
	intltool-merge 		\
	intltool-update


ACLOCAL_AMFLAGS = -I m4
