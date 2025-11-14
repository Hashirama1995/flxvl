cmake_minimum_required(VERSION 3.15)
project(FlexValue VERSION 2.0.0 LANGUAGES CXX)

option(FLEXVALUE_BUILD_TESTS "Build FlexValue tests" ON)
option(FLEXVALUE_BUILD_EXAMPLES "Build FlexValue examples" ON)
option(FLEXVALUE_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(warnings OPTIONAL)

add_subdirectory(src)

if(FLEXVALUE_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

#if(FLEXVALUE_BUILD_EXAMPLES)
#  add_subdirectory(examples)
#endif()

include(GNUInstallDirs)
install(DIRECTORY include/flexvalue
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hpp")
