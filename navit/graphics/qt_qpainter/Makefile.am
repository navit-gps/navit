include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @QT_GUI_CFLAGS@ @QT_SVG_CFLAGS@ @NAVIT_CFLAGS@ -I$(top_srcdir) -I$(top_srcdir)/navit -I$(top_srcdir)/navit/graphics/qt_qpainter -DMODULE=graphics_qt_qpainter
modulegraphics_LTLIBRARIES = libgraphics_qt_qpainter.la
libgraphics_qt_qpainter_la_SOURCES = graphics_qt_qpainter.cpp graphics_qt_qpainter.h RenderArea.cpp RenderArea.h RenderArea.moc
libgraphics_qt_qpainter_la_LDFLAGS = @QT_GUI_LIBS@ @QT_SVG_LIBS@ -module -avoid-version
if GRAPHICS_QT_QPAINTER
BUILT_SOURCES = RenderArea.moc
RenderArea.moc:
	@MOC@ $(srcdir)/RenderArea.h >RenderArea.moc || touch RenderArea.moc

distclean-local:
	$(RM) RenderArea.moc
endif
