# Building the Project

This document describes how to build `czi-pyramidizer` from source.

## Prerequisites

- CMake >=3.15 (project tested with newer versions)
- C++17 capable compiler (MSVC, Clang, or GCC)
- Ninja (recommended generator)
- OpenCV (development headers / libs) if using OpenCV-based decimation
- libCZI and its dependencies
- (Optional) Azure SDK components when building Azure stream support

## Clone

```
git clone https://github.com/ZEISS/czi-pyramidizer.git
cd czi-pyramidizer
```

## Configure

```
cmake -S . -B build -G Ninja \
 -DCMAKE_BUILD_TYPE=Release \
 -DLIBCZI_BUILD_CURL_BASED_STREAM=ON \
 -DLIBCZI_BUILD_AZURESDK_BASED_STREAM=ON
```

Adjust flags as needed.

## Build

```
cmake --build build -j
```

## Run Unit Tests

```
ctest --test-dir build -C Release --output-on-failure
```

## Common Options

| CMake Option | Purpose |
|--------------|---------|
| `CMAKE_BUILD_TYPE` | Build type (Release/Debug/RelWithDebInfo) |
| `LIBCZI_BUILD_CURL_BASED_STREAM` | Enable curl-based streaming |
| `LIBCZI_BUILD_AZURESDK_BASED_STREAM` | Enable Azure SDK streaming |
| `LIBCZI_BUILD_PREFER_EXTERNALPACKAGE_LIBCURL` | Prefer external libcurl |

## Adding a New Source File

1. Place it under `libpyramidizer/src` or appropriate module.
2. Update the corresponding `CMakeLists.txt` if not using globbing.
3. Reconfigure & rebuild.

## Windows Notes

- Ensure `vcpkg` toolchain file is passed if using vcpkg packages.
- WIC decoder is enabled to handle specific image formats.

## Linux Notes

- Install development packages for OpenCV & OpenSSL.
- Use `LD_LIBRARY_PATH` or rpath if running from build tree with shared libs.

## Static vs Dynamic

To favor static link for libstdc++/libgcc on Linux (example):
```
-DCMAKE_CXX_FLAGS="-static-libstdc++ -static-libgcc"
```

## Cleaning

```
cmake --build build --target clean
rm -rf build
```

## Troubleshooting

| Issue | Resolution |
|-------|------------|
| Missing OpenCV headers | Verify `pkg-config --libs opencv4` or correct install path. |
| Link errors with Azure SDK | Ensure matching triplet in vcpkg and consistent build type. |
| Wrong compiler picked | Pass `-DCMAKE_CXX_COMPILER=` explicitly. |

## Building Documentation Locally

The Markdown documentation in `docs/` is built with MkDocs (Material theme).

### Prerequisites

- Python3.x
- pip

### Install MkDocs and plugins

```
pip install --upgrade pip
pip install mkdocs mkdocs-material pymdown-extensions
```

### Live preview (auto-reload)

```
mkdocs serve
```

Open the shown localhost URL (default: http://127.0.0.1:8000).

### Build static site

```
mkdocs build --strict
```

Output is written to the `site/` directory.

### Validate navigation

If you add a new `.md` file, also add it to `mkdocs.yml` under `nav:` or MkDocs will ignore it.

### Clean generated site

```
rm -rf site
```

## Next Steps

- See `dev/contributing.md` for contribution guidelines.
