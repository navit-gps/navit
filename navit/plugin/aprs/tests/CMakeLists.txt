# Unit tests for APRS plugin

# Test executable for database operations
add_executable(test_aprs_db
    test_aprs_db.c
    ../aprs_db.c
)

target_link_libraries(test_aprs_db
    ${SQLITE3_LIBRARIES}
    glib-2.0
    m
)

target_include_directories(test_aprs_db PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${SQLITE3_INCLUDE_DIRS}
)

# Test executable for packet decoding
add_executable(test_aprs_decode
    test_aprs_decode.c
    ../aprs_decode.c
)

target_link_libraries(test_aprs_decode
    glib-2.0
)

target_include_directories(test_aprs_decode PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# The DSP and SDR integration tests moved to plugin/tests_integration/aprs_sdr/
# because they exercise the seam between aprs and aprs_sdr. Keeping them out of
# here lets aprs/tests build with zero references to aprs_sdr.

# Add test targets
enable_testing()

add_test(NAME aprs_db_test COMMAND test_aprs_db)
add_test(NAME aprs_decode_test COMMAND test_aprs_decode)

