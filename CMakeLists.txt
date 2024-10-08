cmake_minimum_required(VERSION 3.18)
project(zserio-cmake-helper)

# Add a target to which zserio libs are added
add_custom_target(${PROJECT_NAME} ALL
  COMMENT "Target to which add_zserio_library is appending.")


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

  message("=> zserio.jar is now available from ${CMAKE_CURRENT_BINARY_DIR}/zserio.jar")

elseif(DEFINED ZSERIO_REPO_ROOT)
  # Build zserio.jar from source
  message("=> Building zserio.jar from source ...")
  message("=> Using zserio from ${ZSERIO_REPO_ROOT}")

  # Setup build/install locations for zserio jar files in the CMake build folder
  set(ZSERIO_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/zserio)
  set(ZSERIO_BUILD_DIR ${ZSERIO_OUTPUT_DIR}/build)
  set(ZSERIO_DISTR_DIR ${ZSERIO_OUTPUT_DIR}/distr)

  # Run ant for zserio_core, zserio_cpp and zserio_doc, then bundle into zserio.jar
  message("=> Running main zserio install ...")
  execute_process(
    COMMAND bash "${ZSERIO_REPO_ROOT}/scripts/build.sh" core cpp zserio -o "${ZSERIO_OUTPUT_DIR}"
    WORKING_DIRECTORY "${ZSERIO_REPO_ROOT}"
    RESULT_VARIABLE ZSERIO_BUILD_RESULT)

  # Make sure the previous commands were successful
  if (NOT ZSERIO_BUILD_RESULT EQUAL "0")
    message(FATAL_ERROR "Failed to build zserio.jar in ${ZSERIO_REPO_ROOT}.")
  endif()

  # Assume location of bundled result jar-file, store as target property
  set_target_properties(${PROJECT_NAME}
    PROPERTIES
      jar "${ZSERIO_DISTR_DIR}/zserio.jar")

  message("=> zserio.jar is now available from ${ZSERIO_DISTR_DIR}/zserio.jar")
else()
  message(FATAL_ERROR "Either ZSERIO_REPO_ROOT or ZSERIO_VERSION must be defined to use zserio.")
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
  if(DEFINED ZSERIO_REPO_ROOT)
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
    # Get the runtime from the zserio repository
    message(STATUS "ZSERIO_VERSION is defined, downloading zserio runtime from Github...")
    if (NOT TARGET ZserioCppRuntime)
      FetchContent_Declare(
        ZserioCppRuntime
        GIT_REPOSITORY https://github.com/ndsev/zserio.git
        GIT_TAG        "v${ZSERIO_VERSION}"
        GIT_SHALLOW    TRUE 
        SOURCE_SUBDIR  compiler/extensions/cpp/runtime/src
      )
      FetchContent_MakeAvailable(ZserioCppRuntime)
    endif()
    message(STATUS "Downloading zserio runtime from Github... complete!")
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
    "EXCLUDE_FROM_ALL;WITH_REFLECTION;WITHOUT_SQL;SHARED;WITH_IMPLICIT_ARRAYS;QUIET;WITH_POLYMORPHIC_ALLOC;CACHE_GENERATION"
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

    list(APPEND zserio_args "-withoutCrossExtensionCheck")
    list(APPEND zserio_args "${ZS_LIB_EXTRA_ARGS}")

    if (ZS_LIB_QUIET)
      set(quiet OUTPUT_QUIET ERROR_QUIET)
    endif()

    message("=> Generating code for zserio library ${ZS_LIB_NAME} ...")
    execute_process(
      COMMAND ${ZSERIO}
        -cpp ${ZSERIO_GEN_DIR}
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
