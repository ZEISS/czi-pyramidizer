// SPDX-FileCopyrightText: 2024 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#pragma once

#include <czi-pyramidizer_config.h>

#include <cstdint>
#include <memory>

#include <libpyramidizer.h>

#if CZIPYRAMIDIZER_WIN32_ENVIRONMENT
#include <Windows.h>
#endif

/// Implementation of the ILog interface that writes to the console. This
/// implementation is intended to provide platform independent ability to change
/// the color of the text on the console. This includes older Windows version
/// that do not support ANSI escape sequences (aka Virtual Terminal Sequences,
/// c.f. https://learn.microsoft.com/en-us/windows/console/classic-vs-vt).
/// If stdout/stderr is redirected to a file, the color information is ignored.
class ConsoleIo : public libpyramidizer::IConsoleIo
{
private:
#if CZIPYRAMIDIZER_WIN32_ENVIRONMENT
    HANDLE console_handle_;
    std::uint16_t default_console_color_;
    bool can_use_virtual_terminal_sequences_;
#endif
#if CZIPYRAMIDIZER_UNIX_ENVIRONMENT
    bool is_terminal_output_;
#endif

public:
    ConsoleIo();

    bool IsStdOutATerminal() const override;

    void SetColor(libpyramidizer::ConsoleColor foreground, libpyramidizer::ConsoleColor background) override;

    void MoveUp(int lines_to_move_up) override;

    void WriteLineStdOut(const char* str) override;
    void WriteLineStdOut(const wchar_t* str) override;
    void WriteLineStdErr(const char* str) override;
    void WriteLineStdErr(const wchar_t* str) override;

    void WriteStdOut(const char* str) override;
    void WriteStdOut(const wchar_t* str) override;
    void WriteStdErr(const char* str) override;
    void WriteStdErr(const wchar_t* str) override;

private:
#if CZIPYRAMIDIZER_WIN32_ENVIRONMENT
    std::uint16_t GetColorAttribute(libpyramidizer::ConsoleColor foreground, libpyramidizer::ConsoleColor background) const;
#endif
#if CZIPYRAMIDIZER_UNIX_ENVIRONMENT
    void SetTextColorAnsi(libpyramidizer::ConsoleColor foreground, libpyramidizer::ConsoleColor background);
#endif
};

std::shared_ptr<libpyramidizer::IConsoleIo> CreateConsoleIo();
