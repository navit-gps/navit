include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @QT_DECLARATIVE_CFLAGS@ @QT_GUI_CFLAGS@ @NAVIT_CFLAGS@ -I$(top_srcdir) -I$(top_srcdir)/navit -DMODULE=gui_qml
modulegui_LTLIBRARIES = libgui_qml.la
libgui_qml_la_SOURCES = gui_qml.moc gui_qml.cpp proxy.moc proxy.h ngqpoint,moc ngqpoint.h
libgui_qml_la_LDFLAGS = @QT_DECLARATIVE_LIBS@ @QT_GUI_LIBS@ -module -avoid-version
BUILT_SOURCES = gui_qml.moc proxy.moc ngqpoint.moc
gui_qml.moc: gui_qml.cpp
	@MOC@ $(srcdir)/gui_qml.cpp >gui_qml.moc
proxy.moc: proxy.h
	@MOC@ $(srcdir)/proxy.h >proxy.moc
ngqpoint.moc: ngqpoint.h
	@MOC@ $(srcdir)/ngqpoint.h >ngqpoint.moc
