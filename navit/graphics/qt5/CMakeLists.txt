# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Find the QtWidgets library
find_package(Qt5Widgets)

module_add_library(graphics_qt5 graphics_qt5.cpp event_qt5.cpp QNavitWidget.cpp)
