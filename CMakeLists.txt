cmake_minimum_required(VERSION 3.18)
project(zserio-cmake-helper)

# Add a target to which zserio libs are added
add_custom_target(${PROJECT_NAME} ALL
  COMMENT "Target to which add_zserio_library is appending.")

# Sanitize JAVA_HOME path to use forward slashes
if(DEFINED ENV{JAVA_HOME})
  string(REPLACE "\\" "/" UNIX_JAVA_HOME "$ENV{JAVA_HOME}")
  string(REGEX REPLACE "^([a-zA-Z]):" "/\\1" UNIX_JAVA_HOME "${UNIX_JAVA_HOME}")
  message(STATUS "Converted JAVA_HOME to Unix-style: ${UNIX_JAVA_HOME}")
  set(ENV{JAVA_HOME} ${UNIX_JAVA_HOME})
endif()

set(ZSERIO_CPP17_PLUGIN_SOURCE_DIR "")

# Figure out zserio-cpp17 module needs to be built from source.
# If ZSERIO_CPP17_VERSION is not a version number, use FetchContent
# to clone the git-ref and set ZSERIO_CPP17_REPO_ROOT to the cloned
# repository.
if (DEFINED ZSERIO_CPP17_VERSION)
  set(VERSION_NUMBER_REGEX "[0-9]+\\.[0-9]+(\\.[0-9]+)?")
  string(REGEX MATCH "${VERSION_NUMBER_REGEX}" ZSERIO_CPP17_VERSION_MATCH "${ZSERIO_CPP17_VERSION}")
  if (NOT ZSERIO_CPP17_VERSION_MATCH)
    set(ZSERIO_CPP17_GIT_TAG "${ZSERIO_CPP17_VERSION}")
    message(STATUS "=> Cloning zserio-cpp17#${ZSERIO_CPP17_VERSION}")
    FetchContent_Declare(ZserioCpp17BuildRootDep
      GIT_REPOSITORY "https://github.com/ndsev/zserio-cpp17.git"
      GIT_TAG        "${ZSERIO_CPP17_GIT_TAG}"
      GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(ZserioCpp17BuildRootDep)
    set(ZSERIO_CPP17_REPO_ROOT "${zseriocpp17buildrootdep_SOURCE_DIR}")
    message(STATUS "=> Setting ZSERIO_CPP17_REPO_ROOT to '${ZSERIO_CPP17_REPO_ROOT}'")
  else()
    set(ZSERIO_CPP17_GIT_TAG "v${ZSERIO_CPP17_VERSION}")
    message(STATUS "=> Using zserio-cpp17#${ZSERIO_CPP17_GIT_TAG}")
  endif()
endif()

if (DEFINED ZSERIO_CPP17_REPO_ROOT)
  if (DEFINED ZSERIO_CPP17_VERSION OR DEFINED ZSERIO_CPP17_ARCHIVE)
    message(STATUS "ZSERIO_CPP17_REPO_ROOT is set; ignoring ZSERIO_CPP17_VERSION/ARCHIVE")
  endif()

  message(STATUS "=> Building zserio-cpp17 from source ...")
  message(STATUS "=> Using zserio-cpp17 from ${ZSERIO_CPP17_REPO_ROOT}")

  set(ZSERIO_CPP17_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/zserio-cpp17")
  set(ZSERIO_CPP17_DISTR_DIR "${ZSERIO_CPP17_OUTPUT_DIR}/distr")

  if(NOT DEFINED ENV{JAVA_HOME})
    message(STATUS "JAVA_HOME is not defined.")
  endif()

  if (NOT BASH_EXECUTABLE)
    find_program(BASH_EXECUTABLE bash)
    if(NOT BASH_EXECUTABLE)
        message(FATAL_ERROR "bash executable not found.")
    endif()
  endif()

  if (NOT EXISTS "${ZSERIO_CPP17_REPO_ROOT}/scripts/build.sh")
    message(FATAL_ERROR "zserio-cpp17 build script not found at ${ZSERIO_CPP17_REPO_ROOT}/scripts/build.sh")
  endif()

  if (NOT DEFINED ZSERIO_CPP17_BUILD_ARGS)
    set(ZSERIO_CPP17_BUILD_ARGS "cpp")
  endif()

  message("=> Running zserio-cpp17 build: '${BASH_EXECUTABLE}' -c \"'${ZSERIO_CPP17_REPO_ROOT}/scripts/build.sh' ${ZSERIO_CPP17_BUILD_ARGS} -o '${ZSERIO_CPP17_OUTPUT_DIR}'\"")
  execute_process(
    COMMAND ${BASH_EXECUTABLE} -c "'${ZSERIO_CPP17_REPO_ROOT}/scripts/build.sh' ${ZSERIO_CPP17_BUILD_ARGS} -o '${ZSERIO_CPP17_OUTPUT_DIR}'"
    WORKING_DIRECTORY "${ZSERIO_CPP17_REPO_ROOT}"
    RESULT_VARIABLE ZSERIO_CPP17_BUILD_RESULT
    ERROR_VARIABLE ZSERIO_CPP17_BUILD_ERROR)

  message(STATUS "Build error: ${ZSERIO_CPP17_BUILD_ERROR}")
  message(STATUS "Build result: ${ZSERIO_CPP17_BUILD_RESULT}")

  if (NOT ZSERIO_CPP17_BUILD_RESULT EQUAL "0")
    message(FATAL_ERROR "Failed to build zserio-cpp17 in ${ZSERIO_CPP17_REPO_ROOT}.")
  endif()

  set(ZSERIO_CPP17_PLUGIN_SOURCE_DIR "${ZSERIO_CPP17_DISTR_DIR}")
else()
  if (DEFINED ZSERIO_CPP17_VERSION)
    if (NOT DEFINED ZSERIO_CPP17_ARCHIVE)
      set(ZSERIO_CPP17_ARCHIVE "${CMAKE_CURRENT_BINARY_DIR}/zserio-cpp17-${ZSERIO_CPP17_VERSION}-bin.zip")
      if (NOT EXISTS "${ZSERIO_CPP17_ARCHIVE}")
        message("=> Downloading zserio-cpp17.jar from GitHub releases ...")
        file(DOWNLOAD
          "https://github.com/ndsev/zserio-cpp17/releases/download/v${ZSERIO_CPP17_VERSION}/zserio-cpp17-${ZSERIO_CPP17_VERSION}-bin.zip"
          "${ZSERIO_CPP17_ARCHIVE}")
      endif()
    endif()
  endif()

  if (DEFINED ZSERIO_CPP17_ARCHIVE)
    set(ZSERIO_CPP17_EXTRACT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    message("=> Unzipping zserio-cpp17.jar (${ZSERIO_CPP17_ARCHIVE})")
    file(ARCHIVE_EXTRACT INPUT "${ZSERIO_CPP17_ARCHIVE}"
      DESTINATION "${ZSERIO_CPP17_EXTRACT_DIR}")
    set(ZSERIO_CPP17_PLUGIN_SOURCE_DIR "${ZSERIO_CPP17_EXTRACT_DIR}")
  endif()
endif()

set(ZSERIO_PLUGIN_DEST_DIR "")

# Depending on whether ZSERIO_REPO_ROOT or ZSERIO_VERSION is set,
# either build zserio.jar from source or download a pre-built version.
if(DEFINED ZSERIO_VERSION)
  set(ZSERIO_ARCHIVE "${CMAKE_CURRENT_BINARY_DIR}/zserio-${ZSERIO_VERSION}-bin.zip")

  # Download pre-built zserio.jar from GitHub releases
  if (NOT EXISTS "${ZSERIO_ARCHIVE}")
    message("=> Downloading zserio.jar from GitHub releases ...")
    file(DOWNLOAD
      "https://github.com/ndsev/zserio/releases/download/v${ZSERIO_VERSION}/zserio-${ZSERIO_VERSION}-bin.zip"
      "${ZSERIO_ARCHIVE}")
  endif()

  # Unzip zserio.jar
  message("=> Unzipping zserio.jar ...")
  file(ARCHIVE_EXTRACT INPUT "${ZSERIO_ARCHIVE}"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

  # Assume location of bundled result jar-file, store as target property
  set_target_properties(${PROJECT_NAME}
    PROPERTIES
      jar "${CMAKE_CURRENT_BINARY_DIR}/zserio.jar")
  set(ZSERIO_PLUGIN_DEST_DIR "${CMAKE_CURRENT_BINARY_DIR}")
  message("=> zserio.jar is now available from ${CMAKE_CURRENT_BINARY_DIR}/zserio.jar")
elseif(DEFINED ZSERIO_REPO_ROOT)
  # Build zserio.jar from source
  message("=> Building zserio.jar from source ...")
  message("=> Using zserio from ${ZSERIO_REPO_ROOT}")

  # Setup build/install locations for zserio jar files in the CMake build folder
  set(ZSERIO_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/zserio)
  set(ZSERIO_BUILD_DIR ${ZSERIO_OUTPUT_DIR}/build)
  set(ZSERIO_DISTR_DIR ${ZSERIO_OUTPUT_DIR}/distr)

  if(NOT DEFINED ENV{JAVA_HOME})
    message(STATUS "JAVA_HOME is not defined.")
  endif()

  if(NOT BASH_EXECUTABLE)
    find_program(BASH_EXECUTABLE bash)
    if(NOT BASH_EXECUTABLE)
        message(FATAL_ERROR "bash not found. Please ensure that bash is installed and accessible.")
    endif()
  endif()

  # Run ant for zserio_core, zserio_cpp, and zserio_doc, then bundle into zserio.jar
  message("=> Running main zserio install: '${BASH_EXECUTABLE}' -c \"'${ZSERIO_REPO_ROOT}/scripts/build.sh' core cpp zserio -o '${ZSERIO_OUTPUT_DIR}'\"")
  execute_process(
    COMMAND ${BASH_EXECUTABLE} -c "'${ZSERIO_REPO_ROOT}/scripts/build.sh' core cpp zserio -o '${ZSERIO_OUTPUT_DIR}'"
    WORKING_DIRECTORY "${ZSERIO_REPO_ROOT}"
    RESULT_VARIABLE ZSERIO_BUILD_RESULT
    ERROR_VARIABLE ZSERIO_BUILD_ERROR)

  message(STATUS "Build error: ${ZSERIO_BUILD_ERROR}")
  message(STATUS "Build result: ${ZSERIO_BUILD_RESULT}")

  # Make sure the previous commands were successful
  if (NOT ZSERIO_BUILD_RESULT EQUAL "0")
    message(FATAL_ERROR "Failed to build zserio.jar in ${ZSERIO_REPO_ROOT}.")
  endif()

  # Assume location of bundled result jar-file, store as target property
  set_target_properties(${PROJECT_NAME}
    PROPERTIES
      jar "${ZSERIO_DISTR_DIR}/zserio.jar")
  set(ZSERIO_PLUGIN_DEST_DIR "${ZSERIO_DISTR_DIR}")

  message("=> zserio.jar is now available from ${ZSERIO_DISTR_DIR}/zserio.jar")
else()
  message(FATAL_ERROR "Either ZSERIO_REPO_ROOT or ZSERIO_VERSION must be defined to use zserio.")
endif()

# Copy the plugin to its destination if the source dir != dest dir
if (DEFINED ZSERIO_CPP17_PLUGIN_SOURCE_DIR AND NOT "${ZSERIO_CPP17_PLUGIN_SOURCE_DIR}" STREQUAL "")
  message(STATUS "Copying C++17 plugin: ${ZSERIO_CPP17_PLUGIN_SOURCE_DIR} → ${ZSERIO_PLUGIN_DEST_DIR}")
  if (NOT "${ZSERIO_CPP17_PLUGIN_SOURCE_DIR}" STREQUAL "${ZSERIO_PLUGIN_DEST_DIR}")
    if (EXISTS "${ZSERIO_CPP17_PLUGIN_SOURCE_DIR}/extensions")
      message(STATUS "Copying extensions/ ...")
      file(COPY "${ZSERIO_CPP17_PLUGIN_SOURCE_DIR}/extensions" DESTINATION "${ZSERIO_PLUGIN_DEST_DIR}")
    endif()
    if (EXISTS "${ZSERIO_CPP17_PLUGIN_SOURCE_DIR}/zserio_cpp17.jar")
      message(STATUS "Copying zserio_cpp17.jar ...")
      file(COPY "${ZSERIO_CPP17_PLUGIN_SOURCE_DIR}/zserio_cpp17.jar" DESTINATION "${ZSERIO_PLUGIN_DEST_DIR}")
    else()
      message(FATAL_ERROR "zserio-cpp17 plugin was requested, but no plugin jar was found in ${ZSERIO_CPP17_PLUGIN_SOURCE_DIR}")
    endif()
  endif()
endif()


# Macro to add zserio-sqlite3 as shipped under zserio/3rdparty
macro(add_zserio_sqlite3)
  if(NOT DEFINED ZSERIO_REPO_ROOT)
    message(FATAL_ERROR "ZSERIO_REPO_ROOT must be defined to use add_zserio_sqlite3")
  endif()

  add_library(zserio-sqlite3
    STATIC
      "${ZSERIO_REPO_ROOT}/3rdparty/cpp/sqlite/sqlite3.c"
      "${ZSERIO_REPO_ROOT}/3rdparty/cpp/sqlite/sqlite3.h")
  target_include_directories(zserio-sqlite3
    PUBLIC
      "${ZSERIO_REPO_ROOT}/3rdparty/cpp/sqlite")
  message("zserio-sqlite3 is now available from ${ZSERIO_REPO_ROOT}/3rdparty/cpp/sqlite")
  target_compile_definitions(zserio-sqlite3
    PRIVATE
      SQLITE_ENABLE_FTS4 SQLITE_ENABLE_FTS5)
  if (UNIX)
    target_link_libraries(zserio-sqlite3 PUBLIC dl)
  endif ()
endmacro()

# Macro to add ZserioCppRuntime
# If the option WITH_SQLITE3 is set, add_zserio_sqlite3
# will be called, and ZserioCppRuntime will link against it.
macro(add_zserio_cpp_runtime)
  if (NOT TARGET ZserioCppRuntime)
    if (DEFINED ZSERIO_CPP17_REPO_ROOT)
      message(STATUS "ZSERIO_CPP17_REPO_ROOT is defined, using zserio cpp17 runtime from ${ZSERIO_CPP17_REPO_ROOT}/runtime/src")
      add_subdirectory("${ZSERIO_CPP17_REPO_ROOT}/runtime/src"
        "./zserio-cpp17-rt")
      add_library(ZserioCppRuntime ALIAS ZserioCpp17Runtime)
    elseif (DEFINED ZSERIO_CPP17_VERSION OR DEFINED ZSERIO_CPP17_ARCHIVE)
      set(ZSERIO_CPP17_RUNTIME_TAG "")
      if (DEFINED ZSERIO_CPP17_GIT_TAG)
        set(ZSERIO_CPP17_RUNTIME_TAG "${ZSERIO_CPP17_GIT_TAG}")
      elseif (DEFINED ZSERIO_CPP17_VERSION)
        string(REGEX MATCH "[0-9]+\\.[0-9]+(\\.[0-9]+)?" ZSERIO_CPP17_VERSION_MATCH "${ZSERIO_CPP17_VERSION}")
        if (NOT ZSERIO_CPP17_VERSION_MATCH)
          set(ZSERIO_CPP17_RUNTIME_TAG "${ZSERIO_CPP17_VERSION}")
        else()
          set(ZSERIO_CPP17_RUNTIME_TAG "v${ZSERIO_CPP17_VERSION}")
        endif()
      else()
        get_filename_component(ZSERIO_CPP17_ARCHIVE_NAME "${ZSERIO_CPP17_ARCHIVE}" NAME)
        string(REGEX MATCH "zserio-cpp17-([0-9]+\\.[0-9]+(\\.[0-9]+)?)" ZSERIO_CPP17_ARCHIVE_MATCH "${ZSERIO_CPP17_ARCHIVE_NAME}")
        if (CMAKE_MATCH_1)
          set(ZSERIO_CPP17_RUNTIME_TAG "v${CMAKE_MATCH_1}")
        else()
          message(FATAL_ERROR "ZSERIO_CPP17_ARCHIVE was provided, but could not infer version from filename. Set ZSERIO_CPP17_VERSION or ZSERIO_CPP17_REPO_ROOT.")
        endif()
      endif()
      if (NOT ZSERIO_CPP17_RUNTIME_TAG)
        message(FATAL_ERROR "Could not determine zserio-cpp17 runtime tag. Set ZSERIO_CPP17_VERSION or ZSERIO_CPP17_REPO_ROOT.")
      endif()
      message(STATUS "URL: https://github.com/ndsev/zserio-cpp17.git#${ZSERIO_CPP17_RUNTIME_TAG}")
      FetchContent_Declare(
        ZserioCpp17RuntimeDep
        GIT_REPOSITORY https://github.com/ndsev/zserio-cpp17.git
        GIT_TAG        "${ZSERIO_CPP17_RUNTIME_TAG}"
        GIT_SHALLOW    TRUE
        SOURCE_SUBDIR  runtime/src)
      FetchContent_MakeAvailable(ZserioCpp17RuntimeDep)
      add_library(ZserioCppRuntime ALIAS ZserioCpp17Runtime)
    elseif (DEFINED ZSERIO_REPO_ROOT)
      message("ZSERIO_REPO_ROOT is defined, using zserio cpp runtime from ${ZSERIO_REPO_ROOT}/compiler/extensions/cpp/runtime/src")
      cmake_parse_arguments(ZS_RUNTIME "WITH_SQLITE3" "" "" ${ARGV0})
      add_subdirectory("${ZSERIO_REPO_ROOT}/compiler/extensions/cpp/runtime/src"
        "./zserio-rt")
      message("Adding zserio cpp runtime from ${ZSERIO_REPO_ROOT}/compiler/extensions/cpp/runtime/src. With SQLite: ${ZS_RUNTIME_WITH_SQLITE3}")
      if (ZS_RUNTIME_WITH_SQLITE3)
        add_zserio_sqlite3()
        target_link_libraries(ZserioCppRuntime PUBLIC zserio-sqlite3)
      endif()
    else()
      message(STATUS "URL: https://github.com/ndsev/zserio.git")
      FetchContent_Declare(
        ZserioCppRuntime
        GIT_REPOSITORY https://github.com/ndsev/zserio.git
        GIT_TAG        "v${ZSERIO_VERSION}"
        GIT_SHALLOW    TRUE
        SOURCE_SUBDIR  compiler/extensions/cpp/runtime/src
      )
      FetchContent_MakeAvailable(ZserioCppRuntime)
    endif()
  else()
    message(STATUS "Target ZserioCppRuntime is already available.")
  endif()
endmacro()

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
#     Set this flag to enable the -withTypeInfoCode and 
#     the -withReflectionCode flags for the zserio C++ emitter.
#   WITHOUT_SQL
#     Set this flag to enable the -withoutSqlCode flag for the
#     zserio C++ emitter.
#   WITH_IMPLICIT_ARRAYS
#     Set this flag to enable the -allowImplicitArrays
#     flag for the zserio C++ emitter.
#   WITH_POLYMORPHIC_ALLOC
#     Set this flag to enable the -setCppAllocator polymorphic
#     flag for the zserio C++ emitter.
#   QUIET
#     Suppress all zserio compiler output.
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
#   CACHE_GENERATION
#     Only run zserio if the target C++ source have not been generated
#     or the specified ZS_LIB_ENTRY changed. Note that changes of other
#     files will not trigger a re-run of zserio!
#
# Example:
#   add_zserio_library(mylib WITH_REFLECTION
#     ROOT path/to/mylib-schema
#     ENTRY mylib.zs)
#
function(add_zserio_library ZS_LIB_NAME)
  cmake_parse_arguments(PARSE_ARGV 0 ZS_LIB
    "EXCLUDE_FROM_ALL;WITH_AMALGAMATION;WITH_REFLECTION;WITHOUT_SQL;SHARED;WITH_IMPLICIT_ARRAYS;QUIET;WITH_POLYMORPHIC_ALLOC;CACHE_GENERATION"
    "ROOT;ENTRY;TOP_LEVEL_PKG"
    "EXTRA_ARGS")

  # Makes sure zserio C++ runtime is added
  if (NOT TARGET ZserioCppRuntime)
    message(FATAL_ERROR "ZserioCppRuntime target is not available! Did you forget to call add_zserio_cpp_runtime()?")
  endif()

  if (NOT ZS_LIB_ROOT)
    message(FATAL_ERROR "Missing zserio-module argument ROOT!")
  elseif (NOT ZS_LIB_ENTRY)
    message(FATAL_ERROR "Missing zserio-module argument ENTRY!")
  endif()

  set(WANTS_REGENERATION YES)
  set(ZSERIO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/${ZS_LIB_NAME}.zserio-gen/")

  # Trigger reconfiguration if the lib's entry file changed
  set_property(SOURCE ${ZS_LIB_ROOT}/${ZS_LIB_ENTRY} APPEND PROPERTY
    CMAKE_CONFIGURE_DEPENDS)

  # Compute the current entry file hash
  file(SHA1 "${ZS_LIB_ROOT}/${ZS_LIB_ENTRY}" NEW_LIB_ENTRY_HASH)
  if (NOT DEFINED ZSERIO_VERSION)
    set(ZSERIO_VERSION "any")
  endif()
  set(NEW_LIB_ENTRY_HASH "${ZSERIO_VERSION}:${NEW_LIB_ENTRY_HASH}:${ARGV}")

  # Compare against the stored entry file hash
  set(LIB_ENTRY_HASH_FILE "${ZSERIO_GEN_DIR}/.entry.sha1")
  if (EXISTS "${LIB_ENTRY_HASH_FILE}" AND NEW_LIB_ENTRY_HASH AND ZS_LIB_CACHE_GENERATION)
    file(READ "${LIB_ENTRY_HASH_FILE}" CUR_LIB_ENTRY_HASH)
    if ("${NEW_LIB_ENTRY_HASH}" STREQUAL "${CUR_LIB_ENTRY_HASH}")
      message(STATUS "=> Hash of ${ZS_LIB_ENTRY} did not change; "
	                   "skipping code generation.")
      set(WANTS_REGENERATION NO)
    endif()
  endif()

  if (${WANTS_REGENERATION})
    file(REMOVE_RECURSE "${ZSERIO_GEN_DIR}")
    file(MAKE_DIRECTORY "${ZSERIO_GEN_DIR}")

    if (NEW_LIB_ENTRY_HASH)
      file(WRITE "${LIB_ENTRY_HASH_FILE}" "${NEW_LIB_ENTRY_HASH}")
    endif()

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

    set(quiet "")
    set(zserio_args "")

    if (ZS_LIB_TOP_LEVEL_PKG)
      list(APPEND zserio_args "-setTopLevelPackage" "${ZS_LIB_TOP_LEVEL_PKG}")
    endif()
    if (ZS_LIB_WITH_REFLECTION)
      list(APPEND zserio_args "-withTypeInfoCode" "-withReflectionCode")
    endif()
    if (ZS_LIB_WITHOUT_SQL)
      list(APPEND zserio_args "-withoutSqlCode")
    endif()
    if (ZS_LIB_WITH_IMPLICIT_ARRAYS)
      list(APPEND zserio_args "-allowImplicitArrays")
    endif()
    if (ZS_LIB_WITH_POLYMORPHIC_ALLOC)
      list(APPEND zserio_args "-setCppAllocator" "polymorphic")
    endif()
    if (ZS_LIB_WITH_AMALGAMATION)
      list(APPEND zserio_args "-withSourcesAmalgamation")
    endif()

    list(APPEND zserio_args "-withoutCrossExtensionCheck")
    list(APPEND zserio_args "${ZS_LIB_EXTRA_ARGS}")

    if (ZS_LIB_QUIET)
      set(quiet OUTPUT_QUIET ERROR_QUIET)
    endif()

    set(CPP_GEN_FLAG "-cpp")
    if (DEFINED ZSERIO_CPP17_VERSION OR DEFINED ZSERIO_CPP17_ARCHIVE OR DEFINED ZSERIO_CPP17_REPO_ROOT)
      set(CPP_GEN_FLAG "-cpp17")
    endif()

    message("=> Generating code for zserio library ${ZS_LIB_NAME} (Generator ${CPP_GEN_FLAG}) ...")
    execute_process(
      COMMAND ${ZSERIO}
        ${CPP_GEN_FLAG} ${ZSERIO_GEN_DIR}
        -src ${ZS_LIB_ROOT}
        ${zserio_args}
        ${ZS_LIB_ENTRY}
      COMMAND_ECHO STDOUT
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
      RESULT_VARIABLE ZSERIO_RESULT
      ${quiet})

    if (NOT ZSERIO_RESULT EQUAL "0")
      message(FATAL_ERROR "/////////// zserio FAILED to generate ${ZS_LIB_NAME} module! ///////////")
    endif()
  endif()

  file(GLOB_RECURSE HEADERS_API "${ZSERIO_GEN_DIR}/*.h")
  file(GLOB_RECURSE SOURCES_API "${ZSERIO_GEN_DIR}/*.cpp")
  set(GENERATED_FILES "${HEADERS_API};${SOURCES_API}")
  set_source_files_properties(${GENERATED_FILES} PROPERTIES GENERATED TRUE)

  if (ZS_LIB_SHARED)
    add_library(${ZS_LIB_NAME} SHARED ${SOURCES_API} ${HEADERS_API})
    set_target_properties(${ZS_LIB_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
  else()
    add_library(${ZS_LIB_NAME} STATIC ${SOURCES_API} ${HEADERS_API})
  endif()

  set_target_properties(${ZS_LIB_NAME} PROPERTIES
    POSITION_INDEPENDENT_CODE YES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO)

  target_include_directories(${ZS_LIB_NAME} PUBLIC "${ZSERIO_GEN_DIR}")
  target_link_libraries(${ZS_LIB_NAME} PUBLIC ZserioCppRuntime)
  target_compile_features (${ZS_LIB_NAME} PUBLIC cxx_std_14)

  if (NOT ZS_LIB_EXCLUDE_FROM_ALL)
    add_dependencies(zserio-cmake-helper ${ZS_LIB_NAME})
  endif()
endfunction()
