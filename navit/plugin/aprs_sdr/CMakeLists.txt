# APRS SDR Plugin - RTL-SDR hardware interface and Bell 202 demodulation

pkg_check_modules(RTLSDR librtlsdr)

if(RTLSDR_FOUND)
    set(plugin_aprs_sdr_LIBS ${RTLSDR_LIBRARIES})
    set(plugin_aprs_sdr_INCLUDES ${RTLSDR_INCLUDE_DIRS})
    set(plugin_aprs_sdr_FLAGS ${RTLSDR_CFLAGS_OTHER})
    add_definitions(-DHAVE_RTLSDR)
    message(STATUS "APRS SDR: RTL-SDR support enabled")
else()
    find_library(RTLSDR_LIBRARIES NAMES rtlsdr)
    find_path(RTLSDR_INCLUDE_DIRS NAMES rtl-sdr.h)
    if(RTLSDR_LIBRARIES AND RTLSDR_INCLUDE_DIRS)
        set(plugin_aprs_sdr_LIBS ${RTLSDR_LIBRARIES})
        set(plugin_aprs_sdr_INCLUDES ${RTLSDR_INCLUDE_DIRS})
        add_definitions(-DHAVE_RTLSDR)
        message(STATUS "APRS SDR: RTL-SDR support enabled")
    else()
        message(STATUS "APRS SDR: RTL-SDR not found - plugin will not be built")
        set_with_reason(plugin/aprs_sdr "RTL-SDR library not found" FALSE)
        return()
    endif()
endif()

module_add_library(plugin_aprs_sdr
    aprs_sdr.c
    aprs_sdr_hw.c
    aprs_sdr_dsp.c
)

# Add pthread for threading
add_definitions(-DHAVE_PTHREAD)

