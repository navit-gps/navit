module_add_library(audio_output-alsa alsa.c)
target_link_libraries(audio_output-alsa ${ALSA_LIBRARY})
