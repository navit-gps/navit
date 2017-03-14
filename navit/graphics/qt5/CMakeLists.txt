# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

module_add_library(graphics_qt5 graphics_qt5.cpp event_qt5.cpp QNavitWidget.cpp)
set(QML 0)
set(QWIDGET 0)
if(USE_QML)
	set(QML 1)
endif ()
if(USE_QWIDGET)
	set(QWIDGET 1)
endif ()

target_compile_definitions(graphics_qt5 PRIVATE USE_QML=${QML} USE_QWIDGET=${QWIDGET})
