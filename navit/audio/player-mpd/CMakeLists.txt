module_add_library(audio_player-mpd mpd.c alsa-audio.c audio.c )
target_link_libraries(audio_player-mpd ${ALSA_LIBRARY})
