supportlib_add_library(support_espeak compiledict.c dictionary.c intonation.c readclause.c setlengths.c 
   numbers.c synth_mbrola.c synthdata.c synthesize.c translate.c tr_languages.c voices.c wavegen.c 
   phonemelist.c klatt.c speak_init.c)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/espeak-data DESTINATION ${SHARE_DIR})
