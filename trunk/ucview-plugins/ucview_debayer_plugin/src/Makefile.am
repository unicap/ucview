plugindir = $(libdir)/ucview/plugins

AM_CPPFLAGS = -I../../src -I$(top_srcdir) -I$(HOME)/opt/include

plugin_LTLIBRARIES = libdebayer_plugin.la

libdebayer_plugin_la_SOURCES = debayer_plugin.c debayer.c debayer.h plugin_ui.h

libdebayer_plugin_la_CFLAGS = @GLIB_CFLAGS@ @UNICAP_CFLAGS@

libdebayer_plugin_la_LIBADD = @GLIB_LIBS@ @UNICAP_LIBS@

libdebayer_plugin_la_LDFLAGS = -module -avoid-version

plugin_ui.h: plugin.gtkbuilder
	python make_header.py plugin.gtkbuilder plugin_ui.h 

plugin.gtkbuilder: plugin.glade
	gtk-builder-convert plugin.glade plugin.gtkbuilder

EXTRA_DIST = \
	make_header.py \
	plugin.glade 
