# Create a library called "Hello" which includes the source file "hello.cxx".
# The extension is already found. Any number of sources could be listed here.

add_definitions( -DMODULE=gui_qml )
QT4_GENERATE_MOC(proxy.h proxy.moc)
QT4_GENERATE_MOC(ngqpoint.h ngqpoint.moc)
QT4_GENERATE_MOC(searchProxy.h searchProxy.moc)
QT4_GENERATE_MOC(bookmarksProxy.h bookmarksProxy.moc)
QT4_GENERATE_MOC(vehicleProxy.h vehicleProxy.moc)
QT4_GENERATE_MOC(navitProxy.h navitProxy.moc)
QT4_GENERATE_MOC(guiProxy.h guiProxy.moc)
QT4_GENERATE_MOC(gui_qml.cpp gui_qml.moc)
add_library(gui_qml ${MODULE_BUILD_TYPE} gui_qml.cpp proxy.moc ngqpoint.moc searchProxy.moc bookmarksProxy.moc vehicleProxy.moc navitProxy.moc guiProxy.moc gui_qml.moc) 

