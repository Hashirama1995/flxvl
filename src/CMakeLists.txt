add_library(flexvalue INTERFACE)

target_include_directories(flexvalue
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:include>
)

if(MSVC)
  target_compile_options(flexvalue INTERFACE /permissive- /Zc:__cplusplus)
else()
  target_compile_features(flexvalue INTERFACE cxx_std_17)
endif()

if(COMMAND set_project_warnings)
    set_project_warnings(flexvalue ${FLEXVALUE_WARNINGS_AS_ERRORS})
endif()

# Больше никакого CodecWorks::flexvalue
