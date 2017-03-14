# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# collect additional source files in this variable
set (GRAPHICS_QT5_ADDITIONAL "")

# initialize QML and QWIDGET usage to not use
set(QML 0)
set(QWIDGET 0)

# check if we have QML
if(USE_QML)
        set(QML 1)
endif ()

#check if we have QWIDGET
if(USE_QWIDGET)
        set(QWIDGET 1)
        set(GRAPHICS_QT5_ADDITIONAL "QNavitWidget.cpp")
endif ()

module_add_library(graphics_qt5 graphics_qt5.cpp event_qt5.cpp ${GRAPHICS_QT5_ADDITIONAL})
# pass QML and QWIDGET preference to source
target_compile_definitions(graphics_qt5 PRIVATE USE_QML=${QML} USE_QWIDGET=${QWIDGET})

# install qml files if QML is used
if(USE_QML)
        install(FILES graphics_qt5.qml DESTINATION ${LIB_DIR}/navit/${graphics_qt5_TYPE}
                                        PERMISSIONS OWNER_READ OWNER_WRITE
                                        OWNER_EXECUTE GROUP_READ GROUP_EXECUTE
                                        WORLD_READ WORLD_EXECUTE)
endif ()
