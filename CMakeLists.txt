project(zserio-cmake-helper)

# Add a target to which zserio libs are added
add_custom_target(${PROJECT_NAME} ALL
  COMMENT "Target to which add_zserio_library is appending.")

# Make sure the path to the zserio git repository root is set
if (NOT DEFINED ZSERIO_REPO_ROOT)
  message(FATAL_ERROR "ZSERIO_REPO_ROOT must be set before add_subdirectory(zserio-cmake-helper)!")
endif()

# Makes sure zserio C++ runtime is added
if (NOT TARGET ZserioCppRuntime)
  message(FATAL_ERROR "ZserioCppRuntime target is not available! Did you forget to call add_subdirectory(.../zserio/compiler/extensions/cpp/runtime/src)?")
endif()

# Find `ant` command to build zserio.jar
find_program(ANT_PATH NAMES ant)
if (NOT ANT_PATH)
  message(FATAL_ERROR "Could not find ant binary!")
endif()

# Setup build/install locations for zserio jar files in the CMake build folder
set(ZSERIO_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/zserio)
set(ZSERIO_BUILD_DIR ${ZSERIO_OUTPUT_DIR}/build)
set(ZSERIO_DISTR_DIR ${ZSERIO_OUTPUT_DIR}/distr)
set(ZSERIO_ANT_PROPS
  "-Dzserio.build_dir=${ZSERIO_BUILD_DIR}"
  "-Dzserio.install_dir=${ZSERIO_DISTR_DIR}"
  "-Dzserio_extensions.build_dir=${ZSERIO_BUILD_DIR}/compiler/extensions"
  "-Dzserio_extensions.install_dir=${ZSERIO_DISTR_DIR}/zserio_libs"
  "-Dzserio_runtimes.build_dir=${ZSERIO_BUILD_DIR}/runtime_libs"
  "-Dzserio_runtimes.install_dir=${ZSERIO_DISTR_DIR}/runtime_libs"
  "-Dzserio_core.jar_file=${ZSERIO_BUILD_DIR}/compiler/core/jar/zserio_core.jar")

# Run ant for zserio_core, zserio_cpp and zserio_doc, then bundle into zserio.jar
message("=> Running main zserio ant install ...")
execute_process(
  COMMAND ${ANT_PATH} ${ZSERIO_ANT_PROPS} install
  WORKING_DIRECTORY "${ZSERIO_REPO_ROOT}"
  RESULT_VARIABLE ZSERIO_ANT_RESULT)
message("=> Running zserio cpp extension ant install ...")
execute_process(
  COMMAND ${ANT_PATH} -f compiler/extensions/cpp ${ZSERIO_ANT_PROPS} install
  WORKING_DIRECTORY "${ZSERIO_REPO_ROOT}"
  RESULT_VARIABLE ZSERIO_ANT_RESULT)
message("=> Running zserio doc extension ant install ...")
execute_process(
  COMMAND ${ANT_PATH} -f compiler/extensions/doc ${ZSERIO_ANT_PROPS} install
  WORKING_DIRECTORY "${ZSERIO_REPO_ROOT}"
  RESULT_VARIABLE ZSERIO_ANT_RESULT)
message("=> Running zserio bundle install ...")
execute_process(
  COMMAND ${ANT_PATH} ${ZSERIO_ANT_PROPS} zserio_bundle.install
  WORKING_DIRECTORY "${ZSERIO_REPO_ROOT}"
  RESULT_VARIABLE ZSERIO_ANT_RESULT)

# Make sure the previous commands were successful
if (NOT ZSERIO_ANT_RESULT EQUAL "0")
  message(FATAL_ERROR "Failed to build zserio.jar in ${ZSERIO_REPO_ROOT}.")
endif()

# Assume location of bundled result jar-file, store as target property
set_target_properties(${PROJECT_NAME}
  PROPERTIES
    jar "${ZSERIO_DISTR_DIR}/zserio.jar")

# Function to create a library from zserio schema sources.
#
# The following arguments are supported:
#   ZS_LIB_NAME
#     Must be the first argument. This will be the name of
#     the newly generated target.
#   EXCLUDE_FROM_ALL
#     Set this flag to prevent that the new target is added
#     as a dependency to the zserio-cmake-helper target.
#   WITH_REFLECTION
#     Set this flag to enable the -withTypeInfoCode flag for
#     the zserio C++ emitter.
#   SHARED
#     Set this flag to create a shared instead of a static lib.
#   ROOT [schema-root-dir]
#     Must be set to the parent directory of your zserio
#     schema entry file.
#   ENTRY [zserio-file]
#     Must be set to the zs file name under [ROOT] which
#     is the entry point of your schema.
#   TOP_LEVEL_PKG [pkg-name]
#     Optional top-level namespace for your schema.
#
# Example:
#   add_zserio_library(mylib WITH_REFLECTION
#     ROOT path/to/mylib-schema
#     ENTRY mylib.zs)
#
function(add_zserio_library ZS_LIB_NAME)
  cmake_parse_arguments(PARSE_ARGV 0
    ZS_LIB "EXCLUDE_FROM_ALL;WITH_REFLECTION;SHARED" "ROOT;ENTRY;TOP_LEVEL_PKG" "")

  if (NOT ZS_LIB_ROOT)
    message(FATAL_ERROR "Missing zserio-module argument ROOT!")
  elseif (NOT ZS_LIB_ENTRY)
    message(FATAL_ERROR "Missing zserio-module argument ENTRY!")
  endif()

  set(ZSERIO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/${ZS_LIB_NAME}.zserio-gen/")
  file(REMOVE_RECURSE "${ZSERIO_GEN_DIR}")
  file(MAKE_DIRECTORY "${ZSERIO_GEN_DIR}")

  find_program(JAVA java)
  if (NOT JAVA)
    message(FATAL_ERROR "Could not find java!")
  endif()

  set(ZSERIO ${JAVA})
  get_target_property(JAR_PATH zserio-cmake-helper jar)
  if (${JAR_PATH} EQUAL "NOTFOUND")
    message(FATAL_ERROR "Could not read property 'jar' from target 'zserio-cmake-helper'.")
  endif()
  list(APPEND ZSERIO "-jar")
  list(APPEND ZSERIO "${JAR_PATH}")

  set(zserio_top_level_arg "")
  set(zserio_reflection_arg "")
  if (ZS_LIB_TOP_LEVEL_PKG)
    set(zserio_top_level_arg "-setTopLevelPackage")
  endif()
  if (ZS_LIB_WITH_REFLECTION)
    set(zserio_reflection_arg "-withTypeInfoCode")
  endif()

  message("=> Generating code for zserio library ${ZS_LIB_NAME} ...")
  execute_process(
    COMMAND ${ZSERIO}
      ${zserio_top_level_arg} ${ZS_LIB_TOP_LEVEL_PKG}
      ${zserio_reflection_arg}
      -cpp ${ZSERIO_GEN_DIR}
      -src ${ZS_LIB_ROOT}
      ${ZS_LIB_ENTRY}
    COMMAND_ECHO STDOUT
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    RESULT_VARIABLE ZSERIO_RESULT)

  if (NOT ZSERIO_RESULT EQUAL "0")
    message(FATAL_ERROR "/////////// zserio FAILED to generate ${ZS_LIB_NAME} module! ///////////")
  endif()

  file(GLOB_RECURSE HEADERS_API "${ZSERIO_GEN_DIR}/*.h")
  file(GLOB_RECURSE SOURCES_API "${ZSERIO_GEN_DIR}/*.cpp")

  if (ZS_LIB_SHARED)
    add_library(${ZS_LIB_NAME} SHARED ${SOURCES_API} ${HEADERS_API})
    set_target_properties(${ZS_LIB_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
  else()
    add_library(${ZS_LIB_NAME} STATIC ${SOURCES_API} ${HEADERS_API})
  endif()

  set_target_properties(${ZS_LIB_NAME} PROPERTIES
    POSITION_INDEPENDENT_CODE YES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO)

  target_include_directories(${ZS_LIB_NAME} PUBLIC "${ZSERIO_GEN_DIR}")
  target_link_libraries(${ZS_LIB_NAME} ZserioCppRuntime)

  if (NOT ZS_LIB_EXCLUDE_FROM_ALL)
    add_dependencies(zserio-cmake-helper ${ZS_LIB_NAME})
  endif()
endfunction()
