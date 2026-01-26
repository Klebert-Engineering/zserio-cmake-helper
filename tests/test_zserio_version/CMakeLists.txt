cmake_minimum_required(VERSION 3.31)
project(ZserioCmakeHelper_VersionTest)

include(FetchContent)

if (NOT DEFINED ZSERIO_TEST_SOURCE_DIR)
  message(FATAL_ERROR "ZSERIO_TEST_SOURCE_DIR is required.")
endif()

set(ZSERIO_VERSION "2.18.0")
add_subdirectory(${ZSERIO_TEST_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/cmake-helper)

add_zserio_cpp_runtime(main)

add_executable(main
  ../main.cpp)

target_link_libraries(main
  PRIVATE ZserioCppRuntime)
