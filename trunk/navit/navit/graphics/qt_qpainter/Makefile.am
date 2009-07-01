include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @QT_GUI_CFLAGS@ @NAVIT_CFLAGS@ -I$(top_srcdir) -I$(top_srcdir)/navit -DMODULE=graphics_qt_qpainter
modulegraphics_LTLIBRARIES = libgraphics_qt_qpainter.la
libgraphics_qt_qpainter_la_SOURCES = graphics_qt_qpainter.cpp graphics_qt_qpainter.moc
libgraphics_qt_qpainter_la_LDFLAGS = @QT_GUI_LIBS@ -module -avoid-version
BUILT_SOURCES = graphics_qt_qpainter.moc
graphics_qt_qpainter.moc:
	@MOC@ $(srcdir)/graphics_qt_qpainter.cpp >graphics_qt_qpainter.moc
