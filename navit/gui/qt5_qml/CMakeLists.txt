# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

qt5_add_resources(GUI_QT5_QML_QRC "gui_qt5_qml.qrc")

module_add_library(gui_qt5_qml gui_qt5_qml.cpp ${GUI_QT5_QML_QRC} backend.cpp qml_map.cpp qml_poi.cpp qml_search.cpp proxy.c)