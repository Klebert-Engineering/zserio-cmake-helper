cmake_minimum_required(VERSION 3.18)
project(zserio-cmake-helper-tests)

include(CTest)

set(build_dir "${CMAKE_CURRENT_BINARY_DIR}")

if (NOT DEFINED ZSERIO_TEST_SOURCE_DIR)
  get_filename_component(ZSERIO_TEST_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()

file(GLOB test_dirs LIST_DIRECTORIES true "${CMAKE_CURRENT_LIST_DIR}/test_*")
foreach (subdir ${test_dirs})
  if (IS_DIRECTORY ${subdir})
    get_filename_component(test_name "${subdir}" NAME)
    add_test(
      NAME "configure_${test_name}"
      COMMAND "${CMAKE_COMMAND}"
        -S "${subdir}"
        -B "${build_dir}/${test_name}"
        "-DZSERIO_TEST_SOURCE_DIR=${ZSERIO_TEST_SOURCE_DIR}")
    add_test(
      NAME "build_${test_name}"
      COMMAND "${CMAKE_COMMAND}"
        --build "${build_dir}/${test_name}")
    set_tests_properties("build_${test_name}" PROPERTIES
      DEPENDS "configure_${test_name}")
  endif()
endforeach()
