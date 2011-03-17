macro(internal_set VARIABLE REASON)
   if (NOT DEFINED ${VARIABLE})
      set(${VARIABLE} ${ARGN})
      set(${VARIABLE}_REASON ${REASON} CACHE STRING "reason")
   else()
      set(${VARIABLE}_REASON "User defined" CACHE STRING "reason")
   endif()
endmacro()

macro(set_with_reason VARIABLE REASON ENABLE)
   if (DEFINED ${VARIABLE}_REASON AND NOT ${VARIABLE}_REASON STREQUAL "User defined")
      set(${VARIABLE} ${ENABLE})
      set(${VARIABLE}_REASON ${REASON})
      string(REPLACE "/" "_" VARIABLE_NAMES ${VARIABLE})
      set(${VARIABLE_NAMES}_LIBS ${ARGN})
   else()
      message("Do not change user defined settings for ${VARIABLE}")
   endif()
endmacro()

macro(add_feature FEATURE REASON ENABLE)
   list(APPEND ALL_FEATURES ${FEATURE})
   internal_set(${FEATURE} ${REASON} ${ENABLE})
endmacro()

macro(cfg_feature FEATURE REASON ENABLE)
   set(${FEATURE} ${ENABLE})
   set(${FEATURE}_REASON ${REASON})
endmacro()

macro(add_module MODULE_PATH REASON ENABLE)
   list(APPEND ALL_MODULES ${MODULE_PATH})
   internal_set(${MODULE_PATH} ${REASON} ${ENABLE})
endmacro()

# plugins are always linked static
macro(add_plugin PLUGIN_PATH REASON ENABLE)
   list(APPEND ALL_PLUGINS ${PLUGIN_PATH})
   internal_set(${PLUGIN_PATH} ${REASON} ${ENABLE})
endmacro()

macro(module_add_library MODULE_NAME )
   add_library(${MODULE_NAME} ${MODULE_BUILD_TYPE} ${ARGN})
   SET_TARGET_PROPERTIES(${MODULE_NAME} PROPERTIES COMPILE_DEFINITIONS "MODULE=${MODULE_NAME}")
   TARGET_LINK_LIBRARIES(${MODULE_NAME} ${${MODULE_NAME}_LIBS})
   SET_TARGET_PROPERTIES( ${MODULE_NAME} PROPERTIES COMPILE_FLAGS "${NAVIT_COMPILE_FLAGS}")
   
   if (ANDROID)
      TARGET_LINK_LIBRARIES(${MODULE_NAME} ${NAVIT_LIBNAME})
   endif()
   if (USE_PLUGINS)
      # workaround to be compatible with old paths
      set_target_properties( ${MODULE_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.libs")

      install(TARGETS ${MODULE_NAME}
              DESTINATION ${LIB_DIR}/navit/${${MODULE_NAME}_TYPE}
              PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
   else()
      TARGET_LINK_LIBRARIES(${MODULE_NAME} ${NAVIT_LIBNAME})
   endif()
endmacro(module_add_library)

macro(supportlib_add_library LIB_NAME )
   add_library(${LIB_NAME} ${ARGN})
   SET_TARGET_PROPERTIES( ${LIB_NAME} PROPERTIES COMPILE_FLAGS "${NAVIT_COMPILE_FLAGS}")
   if (NOT USE_PLUGINS)
      TARGET_LINK_LIBRARIES(${LIB_NAME} ${NAVIT_LIBNAME})
   endif(NOT USE_PLUGINS)
endmacro(supportlib_add_library)

macro(message_error)
   set(NAVIT_DEPENDENCY_ERROR 1)
   message( SEND_ERROR ${ARGN})
endmacro(message_error)
