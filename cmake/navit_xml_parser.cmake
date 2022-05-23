file(STRINGS "${SRC}" TEXT_LINES REGEX "_\\(")
file(WRITE ${DST} "// Strings from navit_shipped.xml\n")

foreach (LINE ${TEXT_LINES})
   string(REGEX REPLACE ".*(_\\(\"[^\"]*\"\\)).*" "\\1\n" OUTPUT_LINE ${LINE})
   file(APPEND ${DST} ${OUTPUT_LINE})
endforeach()

file(READ "${SRC}" SRC_CONTENT)

string(REGEX MATCHALL "<text>([^<>]*)</text>" TEXT_ELEMENTS "${SRC_CONTENT}")

foreach (LINE ${TEXT_ELEMENTS})
   string(REGEX REPLACE ".*<text[^>]*>([^<>]*)</text>.*" "_(\"\\1\")" OUTPUT_LINE ${LINE})
   string(REPLACE "\n" "\\n" OUTPUT_LINE ${OUTPUT_LINE})
   file(APPEND ${DST} "${OUTPUT_LINE}\n")
endforeach()

string(REGEX MATCHALL "<vehicleprofile [^<>]*name=\"[^\"]+\"" ATTRS "${SRC_CONTENT}")

foreach (LINE ${ATTRS})
   string(REGEX REPLACE ".* name=\"([^\"]+)\"" "_(\"\\1\")" OUTPUT_LINE ${LINE})
   file(APPEND ${DST} "${OUTPUT_LINE}\n")
endforeach()

