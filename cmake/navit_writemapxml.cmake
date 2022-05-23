message("${TEXT}")
file(WRITE ${DST} "<map type=\"${TYPE}\" data=\"$NAVIT_SHAREDIR/maps/${DATA}\" />\n")
