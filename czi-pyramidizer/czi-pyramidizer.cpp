// SPDX-FileCopyrightText: 2024 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#include <czi-pyramidizer_config.h>
#include "utilities.h"

#include "consoleio.h"
#include <libpyramidizer.h>


int main(int argc, char* argv[])
{
    // Create a console IO object.
    auto console_io = std::make_shared<ConsoleIo>();

#if CZIPYRAMIDIZER_WIN32_ENVIRONMENT
    CommandlineArgsWindowsHelper args_helper;
    return libpyramidizer::libmain(args_helper.GetArgc(), args_helper.GetArgv(), console_io);
#endif
#if CZIPYRAMIDIZER_UNIX_ENVIRONMENT
    return libpyramidizer::libmain(argc, argv, console_io);
#endif
}
