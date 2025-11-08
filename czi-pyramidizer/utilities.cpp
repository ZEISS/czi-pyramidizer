// SPDX-FileCopyrightText: 2024 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#include "utilities.h"

#if CZIPYRAMIDIZER_WIN32_ENVIRONMENT
#include <memory>
#include <Windows.h>
#endif

#if CZIPYRAMIDIZER_WIN32_ENVIRONMENT

namespace
{
    std::string convertToUtf8(const std::wstring& wide_string)
    {
        if (wide_string.empty())
        {
            return {};
        }

        const int utf8_length = WideCharToMultiByte(
            CP_UTF8,
            0,
            wide_string.data(),
            static_cast<int>(wide_string.size()),
            nullptr,
            0,
            nullptr,
            nullptr);

        if (utf8_length == 0)
        {
            return {};
        }

        std::string utf8_string(utf8_length, '\0');

        if (WideCharToMultiByte(
            CP_UTF8,
            0,
            wide_string.data(),
            static_cast<int>(wide_string.size()),
            utf8_string.data(),
            utf8_length,
            nullptr,
            nullptr) == 0)
        {
            return {};
        }

        return utf8_string;
    }
}

CommandlineArgsWindowsHelper::CommandlineArgsWindowsHelper()
{
    int number_arguments;
    const std::unique_ptr<LPWSTR, decltype(LocalFree)*> wide_argv
    {
        CommandLineToArgvW(GetCommandLineW(), &number_arguments),
        &LocalFree
    };

    this->pointers_to_arguments_.reserve(number_arguments);
    this->arguments_.reserve(number_arguments);

    for (int i = 0; i < number_arguments; ++i)
    {
        this->arguments_.emplace_back(convertToUtf8(wide_argv.get()[i]));
    }

    for (int i = 0; i < number_arguments; ++i)
    {
        this->pointers_to_arguments_.emplace_back(
            this->arguments_[i].data());
    }
}

char** CommandlineArgsWindowsHelper::GetArgv()
{
    return this->pointers_to_arguments_.data();
}

int CommandlineArgsWindowsHelper::GetArgc() const
{
    return static_cast<int>(this->pointers_to_arguments_.size());
}
#endif

