

schemasdir = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = ucview.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@


man_MANS = ucview.1

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = ucview.pc


if GCONF_SCHEMAS_INSTALL
install-data-local:
	        if test -z "$(DESTDIR)" ; then \
	                for p in $(schemas_DATA) ; do \
	                        GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/data/$$p ; \
	                done \
	        fi
else
install-data-local:
endif


EXTRA_DIST = \
	$(man_MANS)	    \
	$(schemas_in_files) \
	ucview.pc.in

CLEANFILES = \
	$(schemas_DATA) \
	$(pkgconfig_DATA)