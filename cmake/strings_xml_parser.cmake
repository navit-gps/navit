file(STRINGS "${SRC}" TEXT_LINES REGEX "<string[ \t]+name=")
file(WRITE ${DST} "// Strings from android/res/values/strings.xml\n\n")

foreach (LINE ${TEXT_LINES})
   string(REGEX REPLACE ".*<string[^>]+>(.*)</string>.*" "\\1" MSGID ${LINE})
   string(REGEX REPLACE "\\\\'" "'" MSGID ${MSGID})
   string(REGEX REPLACE ".*<string[^>]+name=\"([^>\"]+)\">.*</string>.*" "\\1" RESID ${LINE})
   file(APPEND ${DST} "// Android resource: @strings/${RESID}\n_(\"${MSGID}\")\n")
endforeach()

