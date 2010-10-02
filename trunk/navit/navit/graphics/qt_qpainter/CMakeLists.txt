# Create a library called "Hello" which includes the source file "hello.cxx".
# The extension is already found. Any number of sources could be listed here.

add_definitions( -DMODULE=graphics_qt_qpainter )

QT4_GENERATE_MOC(graphics_qt_qpainter.cpp graphics_qt_qpainter.moc)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_library(graphics_qt_qpainter ${MODULE_BUILD_TYPE} graphics_qt_qpainter.moc graphics_qt_qpainter.cpp  ) 

