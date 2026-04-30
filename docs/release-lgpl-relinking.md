# Static LGPL Relinking

This document describes how releases that statically link `libCZI` preserve the
ability to rebuild `czi-pyramidizer` with a modified `libCZI`.

## Scope

`czi-pyramidizer` is released as open source software under the MIT License.
`libCZI` is a ZEISS open source library and may be used by release binaries
under the GNU Lesser General Public License (LGPL).

When a release statically links an LGPL-covered `libCZI`, recipients must be
able to rebuild or relink the application with a modified version of `libCZI`.
For this project, that is handled by publishing the complete application source,
the exact `libCZI` source revision, build configuration, and dependency
information needed to reproduce the release build.

## Required Release Information

Each release should make the following information available:

- the exact `czi-pyramidizer` source revision or release tag
- the exact `libCZI` source revision used by the release
- any local patches applied to `libCZI`
- CMake configuration options used for the release build
- dependency versions and package manager triplets used by the release build
- compiler, operating system, and architecture used for each binary
- build instructions for each supported platform

The current `libCZI` revision is pinned in `modules/libCZI.cmake`.

## Rebuild Procedure

To rebuild `czi-pyramidizer` with a modified `libCZI`, use the following
high-level procedure:

1. Check out the `czi-pyramidizer` release tag.
2. Replace the pinned `libCZI` source with the modified `libCZI` source.
3. Configure CMake with the same options used for the published binary.
4. Build `czi-pyramidizer`.
5. Run the unit tests where supported by the target platform.

The resulting binary is a rebuilt combined work using the modified `libCZI`.

## Current CMake Configuration

The project configures `libCZI` through `modules/libCZI.cmake`. Release builds
disable the `libCZI` command-line tool, dynamic library target, and unit tests:

```cmake
set(LIBCZI_BUILD_CZICMD OFF CACHE BOOL "" FORCE)
set(LIBCZI_BUILD_DYNLIB OFF CACHE BOOL "" FORCE)
set(LIBCZI_BUILD_UNITTESTS OFF CACHE BOOL "" FORCE)
```

The `libCZI` source is fetched with CMake `FetchContent` from the pinned Git
revision in that file.

## Platform Notes

Windows x64 and Windows ARM64 builds use vcpkg static triplets for dependencies.
Ubuntu 24.04 x64 builds use Ubuntu packages and vcpkg for selected dependencies.
Alpine Linux x64 builds compile OpenCV from source to support static linking.

The release workflow is the source of truth for the exact CMake options,
toolchain files, and package manager commands used for each platform.

## License Files

Binary releases should include `LICENSE` and `THIRD_PARTY_LICENSES.txt`.
Releases using LGPL-covered `libCZI` should also make the GNU GPL and GNU LGPL
license texts available, either in the release archive or through the
third-party license disclosure.
