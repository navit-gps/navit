set(speech_iphone_LIBS "-F #${CMAKE_IOS_SDK_ROOT}/System/Library/PrivateFrameworks -framework AVFoundation")
module_add_library(speech_iphone speech_iphone.m VSSpeechSynthesizer.m)
