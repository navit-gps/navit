include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @QT_DECLARATIVE_CFLAGS@ @QT_GUI_CFLAGS@ @QT_XML_CFLAGS@ @NAVIT_CFLAGS@ -I$(top_srcdir) -I$(top_srcdir)/navit -DMODULE=gui_qml
modulegui_LTLIBRARIES = libgui_qml.la
libgui_qml_la_SOURCES = proxy.moc proxy.h ngqpoint.moc ngqpoint.h searchProxy.moc routeProxy.h searchProxy.h bookmarksProxy.moc bookmarksProxy.h vehicleProxy.moc vehicleProxy.h navitProxy.moc navitProxy.h guiProxy.moc guiProxy.h gui_qml.moc gui_qml.cpp
libgui_qml_la_LDFLAGS = @QT_DECLARATIVE_LIBS@ @QT_GUI_LIBS@ @QT_XML_LIBS@ -module -avoid-version
BUILT_SOURCES = gui_qml.moc proxy.moc ngqpoint.moc routeProxy.moc searchProxy.moc bookmarksProxy.moc vehicleProxy.moc navitProxy.moc guiProxy.moc
%.moc: %.h
	@MOC@ $(srcdir)/$< > $@ || touch $@
%.moc: %.cpp
	@MOC@ $(srcdir)/$< > $@ || touch $@
distclean-local:
	-$(RM) *.moc

SUBDIRS=skins
