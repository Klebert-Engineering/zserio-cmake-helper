# zserio-cmake-helper

Provides the `add_zs_library` CMake function. The CMakeLists.txt will
automatically build a `zserio.jar` from you zserio submodule, which will
then be used to generate C++ sources for your schema.

## Usage

In your project's top-level CMakeLists.txt, make sure that the zserio C++
runtime subdirectory is already added. You must then set `ZSERIO_REPO_ROOT`
to the top-level zserio repo path. For example:

```cmake
# Assume zserio submodule is cloned under deps/zserio
add_subdirectory(deps/zserio/compiler/extensions/cpp/runtime/src)
set(ZSERIO_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/deps/zserio")
```

You can then add this project as a subdirectory:

```cmake
add_subdirectory(deps/zserio-cmake-helper)
```

**Important: Make sure that `ant` and `java` are available on your system!**
Otherwise, the above call will fail. Once the `zserio-cmake-helper` directory
is added, the `add_zserio_library` helper function will be available.

```cmake
# Adapt using following parameters:
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

add_zserio_library(mylib WITH_REFLECTION
  ROOT path/to/mylib-schema
  ENTRY mylib.zs)
```
