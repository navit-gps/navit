file(COPY "skins" DESTINATION ".")
add_definitions( -DMODULE=gui_qml )
include_directories(${CMAKE_CURRENT_BINARY_DIR})
QT4_GENERATE_MOC(proxy.h ${CMAKE_CURRENT_BINARY_DIR}/proxy.moc)
QT4_GENERATE_MOC(ngqpoint.h ${CMAKE_CURRENT_BINARY_DIR}/ngqpoint.moc)
QT4_GENERATE_MOC(searchProxy.h ${CMAKE_CURRENT_BINARY_DIR}/searchProxy.moc)
QT4_GENERATE_MOC(bookmarksProxy.h ${CMAKE_CURRENT_BINARY_DIR}/bookmarksProxy.moc)
QT4_GENERATE_MOC(vehicleProxy.h ${CMAKE_CURRENT_BINARY_DIR}/vehicleProxy.moc)
QT4_GENERATE_MOC(navitProxy.h ${CMAKE_CURRENT_BINARY_DIR}/navitProxy.moc)
QT4_GENERATE_MOC(guiProxy.h ${CMAKE_CURRENT_BINARY_DIR}/guiProxy.moc)
QT4_GENERATE_MOC(routeProxy.h ${CMAKE_CURRENT_BINARY_DIR}/routeProxy.moc)
QT4_GENERATE_MOC(gui_qml.cpp ${CMAKE_CURRENT_BINARY_DIR}/gui_qml.moc)
module_add_library(gui_qml gui_qml.cpp
	${CMAKE_CURRENT_BINARY_DIR}/proxy.moc
	${CMAKE_CURRENT_BINARY_DIR}/ngqpoint.moc
	${CMAKE_CURRENT_BINARY_DIR}/searchProxy.moc
	${CMAKE_CURRENT_BINARY_DIR}/bookmarksProxy.moc
	${CMAKE_CURRENT_BINARY_DIR}/vehicleProxy.moc
	${CMAKE_CURRENT_BINARY_DIR}/navitProxy.moc
	${CMAKE_CURRENT_BINARY_DIR}/guiProxy.moc
	${CMAKE_CURRENT_BINARY_DIR}/routeProxy.moc
	${CMAKE_CURRENT_BINARY_DIR}/gui_qml.moc
	)
