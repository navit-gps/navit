file(READ "${SRC}" OUTPUT_LINES)
string(REGEX REPLACE "\"Project-Id-Version: [^\"]*\"" "\"Project-Id-Version: ${PACKAGE_STRING}\\\\n\"" OUTPUT_LINES "${OUTPUT_LINES}")
file(WRITE "${DST}" "${OUTPUT_LINES}")
