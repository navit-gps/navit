module_add_library(audio_player-spotify spotify.c api_keys.c alsa-audio.c audio.c)
target_link_libraries(audio_player-spotify ${ALSA_LIBRARY} ${LIBSPOTIFY_LIBRARIES})
