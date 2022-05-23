set(speech_iphone_LIBS "-F ${CMAKE_OSX_SYSROOT}/System/Library/PrivateFrameworks -framework VoiceServices")
module_add_library(speech_iphone speech_iphone.m)
