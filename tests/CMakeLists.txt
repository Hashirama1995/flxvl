include(FetchContent)
# Для CMake 3.24+ рекомендуем включить NEW поведение:
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(test_flexvalue test_flexvalue.cpp)
target_link_libraries(test_flexvalue PRIVATE flexvalue GTest::gtest_main)

if(COMMAND set_project_warnings)
  set_project_warnings(test_flexvalue ${FLEXVALUE_WARNINGS_AS_ERRORS})
endif()

include(GoogleTest)
gtest_discover_tests(test_flexvalue DISCOVERY_TIMEOUT 60)
