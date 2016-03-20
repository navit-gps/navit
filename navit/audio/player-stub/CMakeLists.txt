module_add_library(audio_player-stub stub.c alsa-audio.c audio.c )
target_link_libraries(audio_player-stub ${ALSA_LIBRARY})
