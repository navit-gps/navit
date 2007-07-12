CATALOGS = @MOFILES@

CLEANFILES = $(CATALOGS) navit.pot

# keep this sorted
EXTRA_DIST = @POFILES@

POTFILES  = \
	$(top_builddir)/src/main.c \
	$(top_builddir)/src/navigation.c \
	$(top_builddir)/src/navit.c \
	$(top_builddir)/src/country.c \
	$(top_builddir)/src/gui/gtk/destination.c \
	$(top_builddir)/src/gui/gtk/gui_gtk_action.c


SUFFIXES = .po .mo

.po.mo: 
	$(MSGFMT) --check --verbose --statistics --output-file=$@ $<

all: @MOFILES@

@POFILES@: navit.pot
	$(MSGMERGE) --update --verbose $@ navit.pot

navit.pot: $(POTFILES)
	 $(XGETTEXT) --default-domain=navit \
	--add-comments --keyword=_ --keyword=_n $(POTFILES)
	if cmp -s navit.po navit.pot; then \
	    rm -f navit.po; \
	else \
	    mv -f navit.po navit.pot; \
	fi

install-data-local:  $(CATALOGS)
	$(mkinstalldirs) $(DESTDIR)$(datadir)/locale
	chmod 755 $(DESTDIR)$(datadir)/locale 
	for n in $(CATALOGS) __DuMmY ; do \
		if test "$$n" -a "$$n" != "__DuMmY" ; then \
			l=`basename $$n .mo`; \
			$(mkinstalldirs) $(DESTDIR)$(datadir)/locale/$$l/LC_MESSAGES; \
			chmod 755 $(DESTDIR)$(datadir)/locale/$$l; \
			chmod 755 $(DESTDIR)$(datadir)/locale/$$l/LC_MESSAGES; \
			$(INSTALL_DATA) -m 644 $$n $(DESTDIR)$(datadir)/locale/$$l/LC_MESSAGES/navit.mo; \
		fi; \
	done
