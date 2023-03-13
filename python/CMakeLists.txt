project(zserio-cmake-helper)

include(FetchContent)

# Add a target to which zserio libs are added
add_custom_target(${PROJECT_NAME} ALL
  COMMENT "Target to which add_zserio_library is appending.")

########

# Make sure the zserio version is set
if (NOT DEFINED ZSERIO_VERSION)
  message(FATAL_ERROR "ZSERIO_VERSION must be set before add_subdirectory(zserio-cmake-helper)!")
endif()

# Find the Python interpreter
find_package(
        Python3
        REQUIRED
        COMPONENTS Interpreter)

message("Using Python3 interpreter at: ${Python3_EXECUTABLE}")

# Check if the zserio package is installed and has the correct version
execute_process(
        COMMAND "${Python3_EXECUTABLE}" -c "import pkg_resources; exit(0) if pkg_resources.get_distribution('zserio').version == '${ZSERIO_VERSION}' else exit(1)"
        RESULT_VARIABLE EXIT_CODE)

if (NOT ${EXIT_CODE} EQUAL 0)
    message(
            STATUS
            "Zserio package is not installed or has incorrect version. Installing zserio package..."
    )
    execute_process(
            COMMAND "${Python3_EXECUTABLE}" -m pip install "zserio==${ZSERIO_VERSION}"
            RESULT_VARIABLE INSTALL_EXIT_CODE)
    if (NOT ${INSTALL_EXIT_CODE} EQUAL 0)
        message(
                FATAL_ERROR
                "Failed to install required zserio version."
        )
    endif()
endif()

# Add the user's local bin directory to the CMake program search path
list(APPEND CMAKE_PROGRAM_PATH "$ENV{HOME}/.local/bin")

# Find the zserio command-line tool installed by pip
find_program(ZSERIO_CMD zserio)

if(NOT ZSERIO_CMD)
  message(FATAL_ERROR "zserio command-line tool not found")
endif()

message(STATUS "Found zserio command-line tool: ${ZSERIO_CMD}")

set(ZSERIO "${ZSERIO_CMD}" CACHE INTERNAL "path to zserio command")
########


# Get the runtime from the zserio repository
if (NOT TARGET ZserioCppRuntime)
  FetchContent_Declare(
    ZserioCppRuntime
    GIT_REPOSITORY https://github.com/ndsev/zserio.git
    GIT_TAG        "v${ZSERIO_VERSION}" 
    SOURCE_SUBDIR  compiler/extensions/cpp/runtime/src
  )
  FetchContent_MakeAvailable(ZserioCppRuntime)
endif()


# Macro to add ZserioCppRuntime
# If the option WITH_SQLITE3 is set, add_zserio_sqlite3
# will be called, and ZserioCppRuntime will link against it.
macro(add_zserio_cpp_runtime)
  cmake_parse_arguments(ZS_RUNTIME "WITH_SQLITE3" "" "" ${ARGV0})
  message("Adding zserio cpp runtime. With SQLite: ${ZS_RUNTIME_WITH_SQLITE3}")
  if (ZS_RUNTIME_WITH_SQLITE3)
    add_zserio_sqlite3()
    target_link_libraries(ZserioCppRuntime PUBLIC zserio-sqlite3)
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
#     Set this flag to enable the -withTypeInfoCode flag for
#     the zserio C++ emitter.
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
#
# Example:
#   add_zserio_library(mylib WITH_REFLECTION
#     ROOT path/to/mylib-schema
#     ENTRY mylib.zs)
#
function(add_zserio_library ZS_LIB_NAME)
  cmake_parse_arguments(PARSE_ARGV 0 ZS_LIB
    "EXCLUDE_FROM_ALL;WITH_REFLECTION;WITHOUT_SQL;SHARED;WITH_IMPLICIT_ARRAYS;QUIET;WITH_POLYMORPHIC_ALLOC"
    "ROOT;ENTRY;TOP_LEVEL_PKG" "")

  # Makes sure zserio C++ runtime is added
  if (NOT TARGET ZserioCppRuntime)
    message(FATAL_ERROR "ZserioCppRuntime target is not available! Did you forget to call add_zserio_cpp_runtime()?")
  endif()

  if (NOT ZS_LIB_ROOT)
    message(FATAL_ERROR "Missing zserio-module argument ROOT!")
  elseif (NOT ZS_LIB_ENTRY)
    message(FATAL_ERROR "Missing zserio-module argument ENTRY!")
  endif()

  set(ZSERIO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/${ZS_LIB_NAME}.zserio-gen/")
  file(REMOVE_RECURSE "${ZSERIO_GEN_DIR}")
  file(MAKE_DIRECTORY "${ZSERIO_GEN_DIR}")

  set(zserio_top_level_arg "")
  set(zserio_reflection_arg "")
  set(zserio_sql_arg "")
  set(zserio_impl_arr_arg "")
  set(zserio_with_polymorphic_alloc_arg "")
  set(quiet "")
  if (ZS_LIB_TOP_LEVEL_PKG)
    set(zserio_top_level_arg "-setTopLevelPackage")
  endif()
  if (ZS_LIB_WITH_REFLECTION)
    set(zserio_reflection_arg "-withTypeInfoCode" "-withReflectionCode")
  endif()
  if (ZS_LIB_WITHOUT_SQL)
    set(zserio_sql_arg "-withoutSqlCode")
  endif()
  if (ZS_LIB_WITH_IMPLICIT_ARRAYS)
    set(zserio_impl_arr_arg "-allowImplicitArrays")
  endif()
  if (ZS_LIB_QUIET)
    set(quiet OUTPUT_QUIET ERROR_QUIET)
  endif()
  if (ZS_LIB_WITH_POLYMORPHIC_ALLOC)
    set(zserio_with_polymorphic_alloc_arg "-setCppAllocator" "polymorphic")
  endif()
  if (ZS_LIB_WITH_POLYMORPHIC_ALLOC)
    set(zserio_with_polymorphic_alloc_arg "-setCppAllocator" "polymorphic")
  endif()

  message("=> Generating code for zserio library ${ZS_LIB_NAME} ...")
  execute_process(
    COMMAND ${ZSERIO}
      ${zserio_top_level_arg} ${ZS_LIB_TOP_LEVEL_PKG}
      ${zserio_reflection_arg} ${zserio_sql_arg} ${zserio_impl_arr_arg} ${zserio_with_polymorphic_alloc_arg}
      -withoutCrossExtensionCheck
      -cpp ${ZSERIO_GEN_DIR}
      -src ${ZS_LIB_ROOT}
      ${ZS_LIB_ENTRY}
    COMMAND_ECHO STDOUT
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    RESULT_VARIABLE ZSERIO_RESULT
    ${quiet})

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
