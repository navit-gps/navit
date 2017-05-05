# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

module_add_library(speech_qt5_espeak qt5_espeak.cpp Qt5EspeakAudioOut.cpp)
if(INTERNAL_ESPEAK_COMPLETE)
	add_definitions ("-DINTERNAL_ESPEAK=1")
	target_link_libraries(speech_qt5_espeak support_espeak)
else()
	add_definitions ("-DINTERNAL_ESPEAK=0")
endif()
