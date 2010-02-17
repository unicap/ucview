plugindir = $(libdir)/ucview/plugins
INCLUDES = -I../../src -I$(top_srcdir) -I$(HOME)/opt/include


plugin_LTLIBRARIES = libvideoplay.la

libvideoplay_la_SOURCES = plugin.c ucvidplay.c ucvidplay.h

libvideoplay_la_CFLAGS = @MODULES_CFLAGS@ -DENABLE_LOGGING=0 -DLOG_DOMAIN=\"videoplayplugin\"

libvideoplay_la_LIBADD = @MODULES_LIBS@

libvideoplay_la_LDFLAGS = -module -avoid-version



bin_PROGRAMS = ucvidplay

ucvidplay_SOURCES = ucvidplay.c ucvidplay.h testvidplay.c

ucvidplay_LDADD = @MODULES_LIBS@

ucvidplay_CFLAGS = @MODULES_CFLAGS@ -DENABLE_LOGGING=1 -DLOG_DOMAIN=\"videoplayplugin\"
