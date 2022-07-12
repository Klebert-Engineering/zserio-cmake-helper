# zserio-cmake-helper

Provides the `add_zserio_library` CMake function. The CMakeLists.txt will
automatically build a `zserio.jar` from your zserio submodule, which will
then be used to generate C++ sources for your schema.

## Usage

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

**Important: Make sure that `ant` and `java` are available on your system!**
Otherwise, the above call will fail. Once the `zserio-cmake-helper` directory
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
