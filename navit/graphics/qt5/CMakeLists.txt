# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
# Instruct CMake to build QRC if needed
#set(CMAKE_AUTORCC ON) -> unfortunate cmake used in CI doesnt have this

# collect additional source files in this variable
set (GRAPHICS_QT5_ADDITIONAL "")

# initialize QML and QWIDGET usage to not use
set(QML 0)
set(QWIDGET 0)
set(HAVE_FREETYPE 0)

# check if we have FREETYPE
if(font/freetype)
	set(HAVE_FREETYPE 1)
endif()

# check if we have QML
if(USE_QML)
	set(QML 1)
	qt5_add_resources(QT5_QRC "graphics_qt5.qrc")
	set(GRAPHICS_QT5_ADDITIONAL ${GRAPHICS_QT5_ADDITIONAL} "QNavitQuick.cpp" ${QT5_QRC})
endif ()

#check if we have QWIDGET
if(USE_QWIDGET)
	set(QWIDGET 1)
	set(GRAPHICS_QT5_ADDITIONAL ${GRAPHICS_QT5_ADDITIONAL} "QNavitWidget.cpp")
endif ()

module_add_library(graphics_qt5 graphics_qt5.cpp event_qt5.cpp ${GRAPHICS_QT5_ADDITIONAL})
# pass QML and QWIDGET preference to source
target_compile_definitions(graphics_qt5 PRIVATE USE_QML=${QML} USE_QWIDGET=${QWIDGET} HAVE_FREETYPE=${HAVE_FREETYPE})

