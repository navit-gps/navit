module_add_library(graphics_qt_qpainter ${CMAKE_CURRENT_BINARY_DIR}/graphics_qt_qpainter.moc graphics_qt_qpainter.cpp)
QT4_GENERATE_MOC(graphics_qt_qpainter.cpp ${CMAKE_CURRENT_BINARY_DIR}/graphics_qt_qpainter.moc)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

