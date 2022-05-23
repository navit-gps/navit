message("Downloading ${URL}")
file(DOWNLOAD ${URL} ${DST} SHOW_PROGRESS STATUS DOWNLOAD_STATUS)
list(GET DOWNLOAD_STATUS 0 DOWNLOAD_ERR)
list(GET DOWNLOAD_STATUS 1 DOWNLOAD_MSG)
if(DOWNLOAD_ERR)
  file(REMOVE ${DST})
  message(SEND_ERROR "Download of sample map from ${URL} failed: "
  "${DOWNLOAD_MSG}\n"
  "To disable the sample map, run cmake with -DSAMPLE_MAP=n .")
endif(DOWNLOAD_ERR)
