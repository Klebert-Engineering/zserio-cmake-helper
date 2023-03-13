# zserio-cmake-helper

This repository provides two different CMake configurations to include zserio into any C++ project. These two configurations target the following use cases:

- Build zserio generator and runtime from source 
- Use pre-built version of zserio compiler and runtime

Building from source is helpful if you need to check out latest changes in the [zserio](https://github.com/ndsev/zserio) repository or want to experiment with your own zserio sources.

The pre-built version is using the zserio compiler provided for Python3 and the versioned runtime directly from the zserio main repo. It is recommended for all developments that can stick with a dedicated version, as it does not need a JDK and Ant installed and uses thoroughly tested release versions of zserio.

The `CMakeLists.txt` in the root directory of this repository is to be used if building from source and is maintained in root for backward compatibility reasons.

The `CMakeLists.txt` in the `python` folder is to be used for the versioned and pre-built config.

Both versions provide the `add_zserio_library` CMake function which generates zserio code based on a provided schema.

## Pre-Requisites

### Option 1: Building zserio from source

As the zserio compiler is built in JAVA, your system needs the following:

- JDK 11 (e.g. `openjdk11`)
- Apache Ant

### Option 2: Pre-Built zserio

The pre-built zserio needs Python3 (3.8.x) and internet access to PyPI to download the appropriate release.

## Usage

### Option 1: Building zserio from source

Add zserio and the zserio-cmake-helper repositories as submodules to your project.

In your project's top-level CMakeLists.txt, set `ZSERIO_REPO_ROOT`
to the top-level zserio repo path, then add the `zserio_cmake_helper`
subdirectory, then call `add_zserio_cpp_runtime()`:

```cmake
# Assume zserio submodule is cloned under deps/zserio
set(ZSERIO_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/deps/zserio")
add_subdirectory(deps/zserio-cmake-helper)
add_zserio_cpp_runtime()

# Now ZserioCppRuntime is available as a target.
# You may also pass WITH_SQLITE3 if you want to
# link against the sqlite sources that are shipped
# inside of the zserio repo under 3rdparty/sqlite3.
```

### Option 2: Pre-Built zserio

This variant uses CMake's FetchContent module to add the required configurations.

In your project's top-level CMakeLists.txt, set `ZSERIO_VERSION`
to the desired zserio version, then add the `zserio_cmake_helper`
project via FetchContent, then call `add_zserio_cpp_runtime()`:

```cmake
# Set zserio version and fetch helper from Github
include(FetchContent)
set(ZSERIO_VERSION "2.10.0")

if (NOT TARGET zserio-cmake-helper)
  FetchContent_Declare(
    zserio-cmake-helper
    GIT_REPOSITORY https://github.com/Klebert-Engineering/zserio-cmake-helper.git
    GIT_TAG        "e2517d463a08e802ec0dbd2972473230455ac9cc" # update if necessary
    SOURCE_SUBDIR  python
  )
  FetchContent_MakeAvailable(zserio-cmake-helper)
endif()

add_zserio_cpp_runtime()
```

In contrast to the option to build from source, you will have to provide SQLite if you whish to use the SQLite options of zserio. This can be done for example by adding it via Conan or by simply copying the sources.

### Both options: `add_zserio_library`

Once the `zserio-cmake-helper` directory
is added, the `add_zserio_library` helper function will be available.

```cmake
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
add_zserio_library(mylib WITH_REFLECTION
  ROOT path/to/mylib-schema
  ENTRY mylib.zs)
```
