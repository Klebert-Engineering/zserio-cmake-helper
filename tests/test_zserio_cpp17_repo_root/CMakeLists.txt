cmake_minimum_required(VERSION 3.31)
project(ZserioCmakeHelper_VersionTest)

include(FetchContent)

if (NOT DEFINED ZSERIO_TEST_SOURCE_DIR)
  message(FATAL_ERROR "ZSERIO_TEST_SOURCE_DIR is required.")
endif()

set(ZSERIO_REPO_ROOT "$ENV{ZSERIO_REPO_ROOT}")
set(ZSERIO_CPP17_REPO_ROOT "$ENV{ZSERIO_CPP17_REPO_ROOT}")
if (NOT DEFINED ENV{ZSERIO_REPO_ROOT})
  message(FATAL_ERROR "Environment variable ZSERIO_REPO_ROOT must be set!")
endif()
if (NOT DEFINED ENV{ZSERIO_CPP17_REPO_ROOT})
  message(FATAL_ERROR "Environment variable ZSERIO_CPP17_REPO_ROOT must be set!")
endif()

add_subdirectory(${ZSERIO_TEST_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/cmake-helper)

add_zserio_cpp_runtime(main)

add_executable(main
  ../main-cpp17.cpp)

target_link_libraries(main
  PRIVATE ZserioCppRuntime)
