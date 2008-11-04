CATALOGS = @MOFILES@

CLEANFILES = $(CATALOGS) navit.pot

# keep this sorted
EXTRA_DIST = @POFILES@

POTFILES  = \
	$(top_srcdir)/navit/main.c \
	$(top_srcdir)/navit/navigation.c \
	$(top_srcdir)/navit/navit.c \
	$(top_srcdir)/navit/popup.c \
	$(top_srcdir)/navit/country.c \
	$(top_srcdir)/navit/gui/gtk/destination.c \
	$(top_srcdir)/navit/gui/gtk/gui_gtk_action.c \
	$(top_srcdir)/navit/gui/gtk/gui_gtk_statusbar.c \
	$(top_srcdir)/navit/gui/internal/gui_internal.c


SUFFIXES = .po .mo

.po.mo: 
	dir=`basename $@ .mo` ;\
	$(MSGFMT) --check --verbose --statistics --output-file=$@ $< && \
	mkdir -p ../locale/$$dir/LC_MESSAGES && \
	cp $@ ../locale/$$dir/LC_MESSAGES/navit.mo

all: @MOFILES@

@POFILES@: navit.pot
	$(MSGMERGE) --no-fuzzy-matching --verbose -o $(top_builddir)/po/$@ $(top_srcdir)/po/$@.in navit.pot

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

uninstall-local:
	for n in $(CATALOGS) __DuMmY ; do \
		if test "$$n" -a "$$n" != "__DuMmY" ; then \
			l=`basename $$n .mo`; \
			rm -f $(DESTDIR)$(datadir)/locale/$$l/LC_MESSAGES/navit.mo; \
		fi; \
	done

distclean-local:
	for n in $(CATALOGS) __DuMmY ; do \
		if test "$$n" -a "$$n" != "__DuMmY" ; then \
			l=`basename $$n .mo`; \
			rm -f ../locale/$$l/LC_MESSAGES/navit.mo; \
			rm -f $$l.po; \
		fi; \
	done
