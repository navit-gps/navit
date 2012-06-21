file(STRINGS "${SRC}" TEXT_LINES REGEX "_\\(")
file(WRITE ${DST} "// Strings from navit_shipped.xml\n")

foreach (LINE ${TEXT_LINES})
   string(REGEX REPLACE ".*(_\\(\"[^\"]*\"\\)).*" "\\1\n" OUTPUT_LINE ${LINE})
   file(APPEND ${DST} ${OUTPUT_LINE})
endforeach()
