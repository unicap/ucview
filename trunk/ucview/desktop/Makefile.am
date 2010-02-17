## Process this file with automake to produce Makefile.in

if DESKTOP_DATADIR

desktopdir = @DESKTOP_DATADIR@/applications
desktop_in_files = ucview.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@


icons48dir = @DESKTOP_DATADIR@/icons/hicolor/48x48/apps
icons48_DATA = 48x48/ucview.png

icons16dir = @DESKTOP_DATADIR@/icons/hicolor/16x16/apps
icons16_DATA = 16x16/ucview.png

install-data-hook:
	touch $(DESTDIR)@DESKTOP_DATADIR@/icons/hicolor

endif


EXTRA_DIST = \
	ucview.desktop          \
	ucview.desktop.in       \
	16x16/ucview.png	\
	48x48/ucview.png
