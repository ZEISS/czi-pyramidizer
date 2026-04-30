# SPDX-FileCopyrightText: 2023 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.15)
 
project(czi-pyramidizer
      VERSION 0.1.3
      HOMEPAGE_URL "https://dev.azure.com/ZEISSgroup/RMS-DEV/_git/czi_pyramidizer"
      DESCRIPTION "generate image-pyramid")

if(WIN32)
  # use "static C-runtime" -> https://stackoverflow.com/questions/14172856/compile-with-mt-instead-of-md-using-cmake
  # Note: this requires CMAKE version 3.15+
  cmake_policy(SET CMP0091 NEW)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif(WIN32)

# This option allows to exclude the unit-tests from the build. The unit-tests are using the
# Google-Test-framework which is downloaded from GitHub during the CMake-run.
option(CZIPYRAMIDIZER_BUILD_UNITTESTS "Build the gTest-based unit-tests" ON)

# When this option is ON, we try to use an existing RapidJSON-library. Otherwise, we download a private
#  copy of RapidJSON during the CMake-run. This option is only relevant if building CZICmd.
option(CZIPYRAMIDIZER_BUILD_PREFER_EXTERNALPACKAGE_RAPIDJSON "Prefer an RapidJSON-package present on the system" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/modules" ${CMAKE_MODULE_PATH})

include("${CMAKE_SOURCE_DIR}/modules/libCZI.cmake")
FetchContent_GetProperties(libCZI)
set(LIBCZI_INCLUDE_DIR "${libczi_SOURCE_DIR}/Src/libCZI")


# Determine whether the Win32-API can be used (in other words: whether we are on a Windows-platform or targetting the Windows-platform).
# Note that just checking for WIN32 is not sufficient here, as this is not defined with environments such as msys2. In those cases, UNIX is defined.
# The meaning of this switch is: whether Win32-API can be used (and this affects not only compilation but e.g. also linking to the Win32-API, as
# it is not automatically linked to when using MinGW or Cygwin).
# TODO(JBL): check whether cross-compiling to Windows works
if (WIN32 OR CYGWIN OR MSYS OR MINGW OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(CZIPYRAMIDIZER_HAVE_WIN32_API ON)
else()
  set(CZIPYRAMIDIZER_HAVE_WIN32_API OFF)
endif()

include(FetchContent)
FetchContent_Declare(
  cli11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11
  GIT_TAG        v2.6.1
)
FetchContent_MakeAvailable(cli11)

FetchContent_Declare(
    GSL
    GIT_REPOSITORY "https://github.com/microsoft/GSL"
    GIT_TAG "v4.2.0"
)
FetchContent_MakeAvailable(GSL)

if (CZIPYRAMIDIZER_BUILD_PREFER_EXTERNALPACKAGE_RAPIDJSON)
    find_package(RapidJSON QUIET)
    if (NOT RapidJSON_FOUND)
        message(FATAL_ERROR [=[
        RapidJSON library was not found, which is required for building. Consider installing
        like 'sudo apt-get install rapidjson-dev'. Alternatively, consider setting the option
        CZIPYRAMIDIZER_BUILD_PREFER_EXTERNALPACKAGE_RAPIDJSON to OFF in order to download and build RapidJSON
        automatically as part of the build process.
        ]=])
    endif()
else()
    # Since "RapidJSON" is a header-only library, we just have to download it and point to the include directory.
    # Note: when using v1.1.0 of RapidJSON (the latest release) there we problems (with GCC14.2 with msys2), so
    #        we use a later version from the master branch.

    # This sets the compatibility policy version minimum to 3.5, allowing
    # modern CMake versions to process the RapidJSON dependency's potentially
    # outdated CMAKE_MINIMUM_REQUIRED syntax without error.
    set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
    FetchContent_Declare(
            RapidJSON
            GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
            GIT_TAG        24b5e7a8b27f42fa16b96fc70aade9106cf7102f # master as of 2025-11-08
            PREFIX "${CMAKE_BINARY_DIR}/vendor/rapidjson"
    )

    set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "" FORCE)
    set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(RapidJSON)

    set(RAPIDJSON_INCLUDE_DIRS ${rapidjson_SOURCE_DIR}/include)
endif()

if (NOT CZIPYRAMIDIZER_HAVE_WIN32_API)
  # with vcpkg, this can be installed with: vcpkg.exe install libiconv 
  find_package(Iconv REQUIRED)
endif()

find_package(OpenCV CONFIG REQUIRED) # on Debian, install via "sudo apt install libopencv-dev"

if (CZIPYRAMIDIZER_BUILD_UNITTESTS)
  enable_testing()
endif()

add_subdirectory(libpyramidizer)
add_subdirectory(czi-pyramidizer)

if (CZIPYRAMIDIZER_BUILD_UNITTESTS)
  add_subdirectory(libpyramidizer-unittests)
endif()
