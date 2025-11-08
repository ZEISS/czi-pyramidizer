# SPDX-FileCopyrightText: 2024 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: MIT

set(CZIPYRAMIDIZER_SOURCES
  "czi-pyramidizer.cpp"
  "utilities.h" 
  "utilities.cpp"
  "consoleio.h"
  "consoleio.cpp")

add_executable(czi-pyramidizer ${CZIPYRAMIDIZER_SOURCES})

set_target_properties(czi-pyramidizer PROPERTIES CXX_STANDARD 17)
target_link_libraries(czi-pyramidizer PRIVATE libpyramidizer)


if (CZIPYRAMIDIZER_HAVE_WIN32_API)
    set(czipyramidizer_UNIX_ENVIRONMENT 0)
    set(czipyramidizer_WIN32_ENVIRONMENT 1)
else()
    set(czipyramidizer_UNIX_ENVIRONMENT 1)
    set(czipyramidizer_WIN32_ENVIRONMENT 0)
endif()

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/czi-pyramidizer_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/czi-pyramidizer_config.h")
target_include_directories(czi-pyramidizer PRIVATE ${CMAKE_CURRENT_BINARY_DIR})