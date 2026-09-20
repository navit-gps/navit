# Integration tests for the aprs <-> aprs_sdr seam.
#
# These tests deliberately pull in headers and source from BOTH plugins, since
# their purpose is to verify the runtime interface between aprs_sdr's DSP and
# aprs's AX.25 decode. This is the one legitimate place for a cross-plugin
# include; aprs/tests and aprs_sdr stay free of references to each other.

# DSP unit tests (exercise aprs_sdr_dsp internals only)
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
    ${CMAKE_CURRENT_SOURCE_DIR}/../../aprs
    ${CMAKE_CURRENT_SOURCE_DIR}/../../aprs_sdr
)

# End-to-end test: synthetic Bell 202 IQ -> aprs_sdr_dsp -> aprs_decode_ax25
add_executable(test_aprs_sdr_integration
    test_aprs_sdr_integration.c
    ../../aprs_sdr/aprs_sdr_dsp.c
    ../../aprs/aprs_decode.c
)

target_link_libraries(test_aprs_sdr_integration
    m
    glib-2.0
)

target_include_directories(test_aprs_sdr_integration PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../aprs
    ${CMAKE_CURRENT_SOURCE_DIR}/../../aprs_sdr
)

enable_testing()

add_test(NAME aprs_dsp_test COMMAND test_aprs_dsp)
add_test(NAME aprs_sdr_integration_test COMMAND test_aprs_sdr_integration)
