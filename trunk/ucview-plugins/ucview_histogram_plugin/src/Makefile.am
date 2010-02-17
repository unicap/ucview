plugindir = $(libdir)/ucview/plugins


INCLUDES = -I../../src -I$(top_srcdir) 

plugin_LTLIBRARIES = libhistogram.la

libhistogram_la_SOURCES = histogram.c

libhistogram_la_CFLAGS = @MODULES_CFLAGS@ -DENABLE_LOGGING=1 -DLOG_DOMAIN=\"histoplugin\"

libhistogram_la_LIBADD = @MODULES_LIBS@ -L.

libhistogram_la_LDFLAGS = -module -avoid-version

gladedir = $(datadir)/ucview/glade
glade_DATA = 
