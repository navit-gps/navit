macro(set_with_reason VARIABLE REASON)
   set(${VARIABLE} ${ARGN})
   set(${VARIABLE}_REASON ${REASON})
endmacro()

# modules may be linked static, or shared for import at runtime
macro(add_module MODULE_PATH)
   list(APPEND ALL_MODULES ${MODULE_PATH})
   set(${MODULE_PATH} ${ARGN})
   set(${MODULE_PATH}_REASON "default")
endmacro()

# plugins are always linked static
macro(add_plugin PLUGIN_PATH)
   list(APPEND ALL_PLUGINS ${PLUGIN_PATH})
   set(${PLUGIN_PATH} ${ARGN})
   set(${PLUGIN_PATH}_REASON "default")
endmacro()

macro(module_add_library MODULE_NAME )
   add_library(${MODULE_NAME} ${MODULE_BUILD_TYPE} ${ARGN}) 
   SET_TARGET_PROPERTIES(${MODULE_NAME} PROPERTIES COMPILE_DEFINITIONS "MODULE=${MODULE_NAME}")
   
   if (USE_PLUGINS)
      SET_TARGET_PROPERTIES( ${LIB_NAME} PROPERTIES COMPILE_FLAGS ${NAVIT_COMPILE_FLAGS})
      TARGET_LINK_LIBRARIES(${MODULE_NAME} navit_core)
      install(TARGETS ${MODULE_NAME}
              DESTINATION ${LIB_DIR}/navit/${${MODULE_NAME}_TYPE}
              PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
   endif()
endmacro(module_add_library)

macro(supportlib_add_library LIB_NAME )
   add_library(${LIB_NAME} ${ARGN}) 
   if (USE_PLUGINS)
      SET_TARGET_PROPERTIES( ${LIB_NAME} PROPERTIES COMPILE_FLAGS ${NAVIT_COMPILE_FLAGS})
      TARGET_LINK_LIBRARIES(${MODULE_NAME} navit_core)
   endif()
endmacro(supportlib_add_library)
