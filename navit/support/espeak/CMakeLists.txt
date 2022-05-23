
if(INTERNAL_ESPEAK_COMPLETE)
	set(ESPEAK_LIBRARY_ADDITIONAL speak_lib.c)
endif()
supportlib_add_library(support_espeak compiledict.c dictionary.c espeak_command.c fifo.c klatt.c
	numbers.c readclause.c sonic.c synthesize.c translate.c  voices.c wavegen.c wave_sada.c
	debug.c espeak.c event.c intonation.c mbrowrap.c phonemelist.c setlengths.c 
	synthdata.c synth_mbrola.c tr_languages.c wave.c wave_pulse.c ${ESPEAK_LIBRARY_ADDITIONAL})

# speak.c 

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/espeak-data DESTINATION ${SHARE_DIR} PATTERN ".svn" EXCLUDE)
