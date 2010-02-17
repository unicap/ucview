## Process this file with automake to produce Makefile.in



INCLUDES = \
	-I$(top_srcdir)	\
	-I$(includedir)


pkginclude_HEADERS = ucview.h ucview-window.h ucview_plugin.h


ucviewdir=$(prefix)/share/ucview



ucview_SOURCES = \
	ucview.c ucview.h \
	ucview-window.c ucview-window.h \
	ucview-about-dialog.c ucview-about-dialog.h \
	callbacks.c callbacks.h \
	checkperm.c checkperm.h \
	ucview-device-dialog.c ucview-device-dialog.h \
	gui.c gui.h  \
	icons.c icons.h \
	ucview-info-box.c ucview-info-box.h \
	marshal.c marshal.h \
	plugin_loader.c plugin_loader.h ucview_plugin.h \
	preferences.c preferences.h \
	settings_dialog.c settings_dialog.h \
	user_config.c user_config.h \
	utils.c utils.h \
	worker.c worker.h \
	sidebar.c sidebar.h \
	ucview-time-selection.c ucview-time-selection.h 

ucview_DATA = ucview.glade

bin_PROGRAMS = ucview

#noinst_LTLIBRARIES = libucview.la

ucview_CFLAGS = @UCVIEW_CFLAGS@ \
		-DINSTALL_PREFIX=\""$(prefix)"\" \
		-DUCVIEW_DATADIR=\""$(datadir)/ucview"\" \
		-DUCVIEW_LOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
		-DUCVIEW_GLADEDIR=\""$(datadir)/ucview/glade"\" \
		-DUCVIEW_PLUGINDIR=\""$(libdir)/ucview/plugins"\" \
		-DUCVIEW_ICONDIR=\""$(datadir)/ucview/icons"\" \
		-DUCVIEW_GLOBAL_CONFIGDIR=\""/etc/ucview"\" \
		-DUCVIEW_VERSION_STRING=\""$(dist_version)"\"

ucview_LDFLAGS = -export-dynamic -no-undefined -export-symbols-regex "^[[^_]].*"

ucview_LDADD = @UCVIEW_LIBS@ -lm 


icons = icon-ucview.h

ucview.c: $(icons)


icon-ucview.h: $(top_srcdir)/icons/ucview-logo.png
	$(GDK_PIXBUF_CSOURCE) --raw --name=icon_ucview_data $< > $(@F)



EXTRA_DIST = ucview.glade ucview.gladep
CLEANFILES = $(icons)


