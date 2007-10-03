CATALOGS = @MOFILES@

CLEANFILES = $(CATALOGS) navit.pot

# keep this sorted
EXTRA_DIST = @POFILES@

POTFILES  = \
	$(top_srcdir)/src/main.c \
	$(top_srcdir)/src/navigation.c \
	$(top_srcdir)/src/navit.c \
	$(top_srcdir)/src/country.c \
	$(top_srcdir)/src/gui/gtk/destination.c \
	$(top_srcdir)/src/gui/gtk/gui_gtk_action.c


SUFFIXES = .po .mo

.po.mo: 
	dir=`basename $@ .mo` ;\
	$(MSGFMT) --check --verbose --statistics --output-file=$@ $< && \
	mkdir -p ../locale/$$dir/LC_MESSAGES && \
	cp $@ ../locale/$$dir/LC_MESSAGES/navit.mo

all: @MOFILES@

@POFILES@: navit.pot
	if test "$(top_srcdir)" != "$(top_builddir)"; then \
		cp $(top_srcdir)/po/$@ $(top_builddir)/po/$@; \
	fi; \
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
