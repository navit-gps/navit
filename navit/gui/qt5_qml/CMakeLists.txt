# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

qt5_add_resources(GUI_QT5_QML_QRC "gui_qt5_qml.qrc")

set(gui_qt5_qml_LIBS graphics_qt5)

module_add_library(gui_qt5_qml gui_qt5_qml.cpp ${GUI_QT5_QML_QRC}
        navitpoimodel.cpp navitrecentsmodel.cpp navitfavouritesmodel.cpp navitsearchmodel.cpp
        navitroute.cpp navitlayoutsmodel.cpp navitlayersmodel.cpp
        navitvehiclesmodel.cpp navitmapsmodel.cpp proxy.c)


#target_link_libraries(gui_qt5_qml graphics_qt5)
