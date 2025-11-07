# Getting Started

Follow these steps to build and run `czi-pyramidizer`.

## 1. Install Prerequisites

- CMake (>=3.15)
- C++17 toolchain
- Ninja (recommended)
- OpenCV development packages
- (Optional) Azure SDK dependencies via vcpkg

## 2. Clone Repository

```
git clone https://github.com/ZEISS/czi-pyramidizer.git
cd czi-pyramidizer
```

## 3. Configure & Build

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## 4. Run

```
./build/czi-pyramidizer/czi-pyramidizer input.czi output.czi
```

## 5. Verify Pyramid Need Only

```
./build/czi-pyramidizer/czi-pyramidizer --check input.czi dummy.czi
```

Exit codes distinguish whether a pyramid is needed.

## 6. Next Steps

- Explore `docs/usage/cli.md`
- Read `docs/dev/build.md`
- Contribute via `docs/dev/contributing.md`

## Troubleshooting

| Problem | Hint |
|---------|------|
| Missing OpenCV headers | Install `libopencv-dev` (Linux) or use vcpkg. |
| Linker errors (Azure) | Ensure matching vcpkg triplet / rebuild cache. |
| Colors not shown | Terminal may not support ANSI; run locally. |

## Cleaning

```
rm -rf build
```

You now have a working setup.
