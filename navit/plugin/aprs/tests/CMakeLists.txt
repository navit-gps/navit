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

# Test executable for DSP
add_executable(test_aprs_dsp
    test_aprs_dsp.c
    ../../aprs_sdr/aprs_sdr_dsp.c
)

target_link_libraries(test_aprs_dsp
    m
    glib-2.0
)

target_include_directories(test_aprs_dsp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../aprs_sdr
)

# Integration test: APRS SDR DSP + AX.25 decode
add_executable(test_aprs_sdr_integration
    test_aprs_sdr_integration.c
    ../../aprs_sdr/aprs_sdr_dsp.c
    ../aprs_decode.c
)

target_link_libraries(test_aprs_sdr_integration
    m
    glib-2.0
)

target_include_directories(test_aprs_sdr_integration PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../aprs_sdr
)

# Add test targets
enable_testing()

add_test(NAME aprs_db_test COMMAND test_aprs_db)
add_test(NAME aprs_decode_test COMMAND test_aprs_decode)
add_test(NAME aprs_dsp_test COMMAND test_aprs_dsp)
add_test(NAME aprs_sdr_integration_test COMMAND test_aprs_sdr_integration)

