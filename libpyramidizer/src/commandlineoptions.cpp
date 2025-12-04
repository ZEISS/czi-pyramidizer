// SPDX-FileCopyrightText: 2024 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#include "../inc/commandlineoptions.h"
#include "libpyramidizer_config.h"

#include <CLI/App.hpp>
#include <CLI/Formatter.hpp>
#include "CLI/ExtraValidators.hpp"
#include <CLI/Config.hpp>
#include <rapidjson/document.h>

#include "../inc/utilities.h"
#include "../inc/ILog.h"

#include "gsl/narrow"

#include <string>

using namespace std;
using namespace libpyramidizer;

namespace libpyramidizer
{
    /// A custom formatter for CLI11 - used to have nicely formatted descriptions.
    class CustomFormatter : public CLI::Formatter
    {
    private:
        static const int DEFAULT_COLUMN_WIDTH = 20;

    public:
        CustomFormatter() { this->column_width(DEFAULT_COLUMN_WIDTH); }

        std::string make_usage(const CLI::App* app, std::string name) const override
        {
            // 'name' is the full path of the executable, we only take the path "after
            // the last slash or backslash"
            size_t offset = name.rfind('/');
            if (offset == string::npos)
            {
                offset = name.rfind('\\');
            }

            if (offset != string::npos && offset < name.length())
            {
                name = name.substr(1 + offset);
            }

            const auto result_from_stock_implementation = this->CLI::Formatter::make_usage(app, name);
            ostringstream usage(result_from_stock_implementation);
            usage << result_from_stock_implementation << endl
                << "  version: " << LIBPYRAMIDIZER_VERSION_MAJOR << "." << LIBPYRAMIDIZER_VERSION_MINOR << "." << LIBPYRAMIDIZER_VERSION_PATCH;
            std::string tweak(LIBPYRAMIDIZER_VERSION_TWEAK);
            if (!tweak.empty())
            {
                usage << "." << tweak;
            }

            usage << endl;
            return usage.str();
        }

        std::string make_option_desc(const CLI::Option* option) const override
        {
            // we wrap the text so that it fits in the "second column"
            const auto lines = wrap(option->get_description().c_str(), 80 - this->get_column_width());

            string options_description;
            options_description.reserve(accumulate(lines.cbegin(), lines.cend(), static_cast<size_t>(0),
                [](size_t sum, const std::string& str) { return 1 + sum + str.size(); }));
            for (const auto& line : lines)
            {
                options_description.append(line).append("\n");
            }

            return options_description;
        }

        static std::vector<std::string> wrap(const char* text, size_t line_length)
        {
            istringstream iss(text);
            std::string word;
            std::string line;
            std::vector<std::string> vec(0, "");

            for (;;)
            {
                iss >> word;
                if (!iss)
                {
                    break;
                }

                // '\n' before a word means "insert a linebreak", and "\N' means "insert a
                // linebreak and one more empty line"
                if (word.size() > 2 && word[0] == '\\' && (word[1] == 'n' || word[1] == 'N'))
                {
                    line.erase(line.size() - 1);  // remove trailing space
                    vec.push_back(line);
                    if (word[1] == 'N')
                    {
                        vec.emplace_back("");
                    }

                    line.clear();
                    word.erase(0, 2);
                }

                if (line.length() + word.length() > line_length)
                {
                    line.erase(line.size() - 1);
                    vec.push_back(line);
                    line.clear();
                }

                line += word + ' ';
            }

            if (!line.empty())
            {
                line.erase(line.size() - 1);
                vec.push_back(line);
            }

            return vec;
        }
    };

    /// CLI11-validator for the option "--compressionopts".
    struct CompressionOptionsValidator : CLI::Validator
    {
        CompressionOptionsValidator()
        {
            this->name_ = "CompressionOptionsValidator";
            this->func_ = [](const string& str) -> string
                {
                    const bool parsed_ok = CommandLineOptions::TryParseCompressionOptions(str, nullptr);
                    if (!parsed_ok)
                    {
                        ostringstream string_stream;
                        string_stream << "Invalid compression-options given \"" << str << "\"";
                        throw CLI::ValidationError(string_stream.str());
                    }

                    return {};
                };
        }
    };

    struct BackgroundColorOptionValidator : CLI::Validator
    {
        BackgroundColorOptionValidator()
        {
            this->name_ = "BackgroundColorOptionValidator";
            this->func_ = [](const string& str) -> string
                {
                    const bool parsed_ok = CommandLineOptions::TryParseBackgroundColorOption(str, nullptr);
                    if (!parsed_ok)
                    {
                        ostringstream string_stream;
                        string_stream << "Invalid background-color option given \"" << str << "\"";
                        throw CLI::ValidationError(string_stream.str());
                    }

                    return {};
                };
        }
    };
} // namespace

// ---------------------------------------------------------------------------

CommandLineOptions::CommandLineOptions(const std::shared_ptr<IConsoleIo>& console_io)
    : console_io_(console_io)
{
}


CommandLineOptions::ParseResult CommandLineOptions::Parse(int argc, char** argv)
{
    const static CompressionOptionsValidator compression_options_validator;
    const static BackgroundColorOptionValidator background_color_validator;

    // specify the string-to-enum-mapping for "mode-of-operation"
    std::map<std::string, ModeOfOperation> map_string_to_mode_of_operation
    {
        { "IfNeeded", ModeOfOperation::OnlyOperateIfPyramidIsNeeded},
        { "Always", ModeOfOperation::AlwaysGeneratePyramid },
    };

    CLI::App app{ argv[0] };
    app.get_formatter()->column_width(40);

    string source_czi_uri;
    string argument_source_stream_class;
    string argument_source_stream_creation_propbag;
    string destination_czi_uri;
    string argument_compression_options;
    int argument_verbosity;
    int argument_top_level_pyramid_size = kDefaultMaxTopLevelPyramidSize;
    int argument_tile_size = kDefaultPyramidTileSize;
    bool argument_overwrite_destination = false;
    string argument_background_color;
    bool argument_version_flag = false;
    bool argument_do_only_check_flag = false;
    int argument_threshold_for_pyramid = 4096;
    ModeOfOperation argument_mode_of_operation = ModeOfOperation::OnlyOperateIfPyramidIsNeeded;

    app.add_option("-s,--source", source_czi_uri, "Source CZI file URI");

    app.add_option("--source-stream-class", argument_source_stream_class,
        "Specifies the stream-class used for reading the source CZI-file. If not specified, the default file-reader stream-class is used."
        " Run with argument '--version' to get a list of available stream-classes.")
        ->option_text("STREAMCLASS");

    app.add_option("--propbag-source-stream-creation", argument_source_stream_creation_propbag,
        "Specifies the property-bag used for creating the stream used for reading the source CZI-file. The data is given in JSON-notation.")
        ->option_text("PROPBAG");

    app.add_option("-d,--destination", destination_czi_uri, "Destination CZI file URI");

    app.add_option("-n,--mode-of-operation", argument_mode_of_operation,
        "Option controlling the mode of operation - can be either 'IfNeeded' meaning that a pyramid generation is "
        "attempted irrespective whether a pyramid is present in the source, or 'Always' which will discard a potentially "
        "existing pyramid. Default is 'IfNeeded'.")
        ->option_text("MODE")
        ->default_val(ModeOfOperation::OnlyOperateIfPyramidIsNeeded)
        ->transform(CLI::CheckedTransformer(map_string_to_mode_of_operation, CLI::ignore_case));

    app.add_option("-c,--compressionopts", argument_compression_options,
        "A string defining the compression options for the pyramid-subblocks created. C.f. below for the syntax.")
        ->option_text("COMPRESSIONOPTIONS")
        ->check(compression_options_validator);

    ostringstream explanation;
    explanation << "The maximum size of the top-level pyramid tile. Pyramid layers are added until the top-level pyramid tile is smaller than this size. Default is " << kDefaultMaxTopLevelPyramidSize << ".";
    app.add_option("-m, --max_top_level_pyramid_size", argument_top_level_pyramid_size,
        explanation.str())
        ->option_text("MAX_TOP_LEVEL_PYRAMID_SIZE")
        ->default_val(kDefaultMaxTopLevelPyramidSize)
        ->check(CLI::NonNegativeNumber);

    explanation = ostringstream();
    explanation << "The extent of the pyramid-tiles in pixels. Default is " << kDefaultPyramidTileSize << ".";
    app.add_option("-t, --tile_size", argument_tile_size,
        explanation.str())
        ->option_text("TILE_SIZE")
        ->default_val(kDefaultPyramidTileSize)
        ->check(CLI::NonNegativeNumber);

    app.add_option("-v,--verbosity", argument_verbosity,
        "Set the verbosity of the console output. The argument is a number between 0 and 5, where higher "
        "number indicate that more output is generated.")
        ->option_text("VERBOSITYLEVEL")
        ->default_val(libpyramidizer::LOGLEVEL_INFORMATION)
        ->check(CLI::Range(0, 5));

    app.add_flag("-o,--overwrite", argument_overwrite_destination,
        "Overwrite the destination file if it exists.");

    app.add_option("-b, --background-color", argument_background_color,
        "Choose the background color, which can be either black or white. Possible values are 'black' or '0', 'white' or '1' and 'auto'. "
        "With 'auto', the appropriate background color is attempted to be detected from the source document.")
        ->option_text("COLOR")
        ->check(background_color_validator);

    app.add_option("-p, --threshold-for-pyramid", argument_threshold_for_pyramid,
        "The threshold when a pyramid is required. If the image is smaller than this threshold, no pyramid is necessary. Default is 4096.")
        ->option_text("THRESHOLD_SIZE")
        ->default_val(4096)
        ->check(CLI::PositiveNumber);

    app.add_flag("--version", argument_version_flag,
        "Print extended version-info and supported operations, then exit.");

    app.add_flag("--check-only", argument_do_only_check_flag,
        "Check the source whether it already contains a pyramid, and exit.");

    app.formatter(make_shared<CustomFormatter>());
    app.footer(CommandLineOptions::GetFooterText());

    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::CallForHelp& e)
    {
        this->DoConsole(
            [&](IConsoleIo* console_io)->void
            {
                console_io->WriteLineStdOut(app.help());
            });
        return ParseResult::Exit;
    }
    catch (const CLI::ParseError& e)
    {
        this->DoConsole(
            [&](IConsoleIo* console_io)->void
            {
                console_io->WriteLineStdOut(e.what());
            });
        return ParseResult::Error;
    }

    if (argument_version_flag)
    {
        this->PrintHelpBuildInfo();
        this->DoConsole(
            [](IConsoleIo* console_io)->void
            {
                console_io->WriteLineStdOut("");
                console_io->WriteLineStdOut("");
            });
        this->PrintHelpStreamsObjects();
        return ParseResult::Exit;
    }

    if (source_czi_uri.empty())
    {
        console_io_->WriteLineStdOut("No source file specified.");
        return ParseResult::Error;
    }

    this->source_czi_uri_ = Utilities::utf8_to_wstring(source_czi_uri);

    this->source_czi_stream_class_ = argument_source_stream_class;

    if (!argument_source_stream_creation_propbag.empty())
    {
        const bool b = TryParseInputStreamCreationPropertyBag(argument_source_stream_creation_propbag, &this->property_bag_for_stream_class_);
        ThrowIfFalse(b, "--propbag-source-stream-creation", argument_source_stream_creation_propbag);
    }

    if (!argument_compression_options.empty())
    {
        libCZI::Utils::CompressionOption compression_options;
        const bool b = TryParseCompressionOptions(argument_compression_options, &compression_options);
        ThrowIfFalse(b, "--compressionopts", argument_compression_options);
        this->compression_mode_ = compression_options.first;
        this->compression_parameters_ = compression_options.second;
    }

    if (!argument_background_color.empty())
    {
        BackgroundColorOption background_color_option;
        const bool b = TryParseBackgroundColorOption(argument_background_color, &background_color_option);
        ThrowIfFalse(b, "--background-color", argument_background_color);
        this->background_color_option_ = background_color_option;
    }

    this->pyramid_tile_size_ = static_cast<std::uint32_t>(argument_tile_size);
    this->max_top_level_pyramid_size_ = static_cast<std::uint32_t>(argument_top_level_pyramid_size);
    this->log_verbosity_level_ = argument_verbosity;
    this->overwrite_destination_ = argument_overwrite_destination;
    this->threshold_when_pyramid_is_required_ = gsl::narrow<std::uint32_t>(argument_threshold_for_pyramid);
    this->mode_of_operation_ = argument_mode_of_operation;

    if (argument_do_only_check_flag)
    {
        this->command_ = Command::CheckOnly;
        return ParseResult::OK;
    }

    if (destination_czi_uri.empty())
    {
        console_io_->WriteLineStdOut("No destination file specified.");
        return ParseResult::Error;
    }

    this->destination_czi_uri_ = Utilities::utf8_to_wstring(destination_czi_uri);

    this->command_ = Command::Pyramidize;
    return ParseResult::OK;
}

/*static*/bool CommandLineOptions::TryParseCompressionOptions(const std::string& s, libCZI::Utils::CompressionOption* compression_option)
{
    try
    {
        const libCZI::Utils::CompressionOption opt = libCZI::Utils::ParseCompressionOptions(s);
        if (compression_option != nullptr)
        {
            *compression_option = opt;
        }

        return true;
    }
    catch (exception&)
    {
        return false;
    }
}

/*static*/void CommandLineOptions::ThrowIfFalse(bool b, const std::string& argument_switch, const std::string& argument)
{
    if (!b)
    {
        ostringstream string_stream;
        string_stream << "Error parsing argument for '" << argument_switch << "' -> \"" << argument << "\".";
        throw runtime_error(string_stream.str());
    }
}

/*static*/std::string CommandLineOptions::GetAppDescription()
{
    int libCZI_version_major, libCZI_version_minor, libCZI_version_patch;
    libCZI::GetLibCZIVersion(&libCZI_version_major, &libCZI_version_minor, &libCZI_version_patch);
    ostringstream string_stream;
    string_stream << "czi-pyramidizer version " << LIBPYRAMIDIZER_VERSION_MAJOR << '.' << LIBPYRAMIDIZER_VERSION_MINOR << '.' << LIBPYRAMIDIZER_VERSION_PATCH;
    string_stream << ", using libCZI version " << libCZI_version_major << '.' << libCZI_version_minor << '.' << libCZI_version_patch << endl;
    return string_stream.str();
}

/*static*/std::string CommandLineOptions::GetFooterText()
{
    static const char* footer_text =
        "This tool is used to create pyramid layers and put them into a CZI file. The image data "
        "from the source CZI is read, from this an image pyramid is constructed, and the source "
        "image and the pyramid is written into an output CZI. "
        "\\nIf the '--compressionopts' argument is given, then the pyramid-subblocks will be compressed "
        "using this specification. If not given, the compression mode is determined by inspecting the "
        "source document. The format (used with the '--compressionopts' argument) is "
        "\"compression_method: key=value; ...\". It starts with the name of the compression-method, followed by a colon, "
        "then followed by a list of key-value pairs which are separated by a semicolon. "
        "\\nExamples: \"zstd0:ExplicitLevel=3\", \"zstd1:ExplicitLevel=2;PreProcess=HiLoByteUnpack\", \"uncompressed:\", \"jpgxr:\". "
        "\\N"
        "The following exit codes are returned from the application: "
        "\\n0 - The operation completed successfully. If running a check (--check-only flag"
        "     given), this indicates that there is no need to generate a pyramid - it is"
        "     either not needed or already present. "
        "\\n1 - Generic failure code - some kind of error occurred, which is not further characterized by the exit code. "
        "\\n10 - In check-only mode: this exit code indicates that a pyramid is needed. "
        "\\n11 - If mode-of-operation=IfNeeded - no action taken as no pyramid was needed. "
        "\\n99 - There was an error parsing the command-line arguments.";

    const auto lines = CustomFormatter::wrap(footer_text, 80);

    ostringstream string_stream;
    for (const auto& line : lines)
    {
        string_stream << line << "\n";
    }

    string_stream << "\n\n";
    int major_version, minor_version, tweak_version;
    libCZI::GetLibCZIVersion(&major_version, &minor_version, &tweak_version);
    libCZI::BuildInformation build_information;
    libCZI::GetLibCZIBuildInformation(build_information);
    string_stream << "libCZI version: " << major_version << "." << minor_version << "." << tweak_version;
    if (!build_information.compilerIdentification.empty())
    {
        string_stream << " (built with " << build_information.compilerIdentification << ")";
    }

    string_stream << endl;
    const int stream_classes_count = libCZI::StreamsFactory::GetStreamClassesCount();
    string_stream << " stream-classes: ";
    for (int i = 0; i < stream_classes_count; ++i)
    {
        libCZI::StreamsFactory::StreamClassInfo stream_info;
        libCZI::StreamsFactory::GetStreamInfoForClass(i, stream_info);
        if (i > 0)
        {
            string_stream << ", ";
        }

        string_stream << stream_info.class_name;
    }

    return string_stream.str();
}

/*static*/bool CommandLineOptions::TryParseInputStreamCreationPropertyBag(const std::string& s, std::map<int, libCZI::StreamsFactory::Property>* property_bag)
{
    // Here we parse the JSON-formatted string that contains the property bag for the input stream and
    //  construct a map<int, libCZI::StreamsFactory::Property> from it.

    int property_info_count;
    const libCZI::StreamsFactory::StreamPropertyBagPropertyInfo* property_infos = libCZI::StreamsFactory::GetStreamPropertyBagPropertyInfo(&property_info_count);

    rapidjson::Document document;
    document.Parse(s.c_str());
    if (document.HasParseError() || !document.IsObject())
    {
        return false;
    }

    for (rapidjson::Value::ConstMemberIterator itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr)
    {
        if (!itr->name.IsString())
        {
            return false;
        }

        string name = itr->name.GetString();
        size_t index_of_key = numeric_limits<size_t>::max();
        for (size_t i = 0; i < static_cast<size_t>(property_info_count); ++i)
        {
            if (name == property_infos[i].property_name)
            {
                index_of_key = i;
                break;
            }
        }

        if (index_of_key == numeric_limits<size_t>::max())
        {
            return false;
        }

        switch (property_infos[index_of_key].property_type)
        {
            case libCZI::StreamsFactory::Property::Type::String:
                if (!itr->value.IsString())
                {
                    return false;
                }

                if (property_bag != nullptr)
                {
                    property_bag->insert(std::make_pair(property_infos[index_of_key].property_id, libCZI::StreamsFactory::Property(itr->value.GetString())));
                }

                break;
            case libCZI::StreamsFactory::Property::Type::Boolean:
                if (!itr->value.IsBool())
                {
                    return false;
                }

                if (property_bag != nullptr)
                {
                    property_bag->insert(std::make_pair(property_infos[index_of_key].property_id, libCZI::StreamsFactory::Property(itr->value.GetBool())));
                }

                break;
            case libCZI::StreamsFactory::Property::Type::Int32:
                if (!itr->value.IsInt())
                {
                    return false;
                }

                if (property_bag != nullptr)
                {
                    property_bag->insert(std::make_pair(property_infos[index_of_key].property_id, libCZI::StreamsFactory::Property(itr->value.GetInt())));
                }

                break;
            default:
                // this actually indicates an internal error - the table property_infos contains a not yet implemented property type
                return false;
        }
    }

    return true;
}

/*static*/bool CommandLineOptions::TryParseBackgroundColorOption(const std::string& str, BackgroundColorOption* background_color_option)
{
    const string trimmed_string = Utilities::toLower(Utilities::trim(str));

    if (trimmed_string == "black" || trimmed_string == "0")
    {
        if (background_color_option != nullptr)
        {
            background_color_option->mode = BackgroundColorOption::Mode::UseFixed;
            background_color_option->white_or_black = false;
        }

        return true;
    }

    if (trimmed_string == "white" || trimmed_string == "1")
    {
        if (background_color_option != nullptr)
        {
            background_color_option->mode = BackgroundColorOption::Mode::UseFixed;
            background_color_option->white_or_black = true;
        }

        return true;
    }

    if (trimmed_string == "auto")
    {
        if (background_color_option != nullptr)
        {
            background_color_option->mode = BackgroundColorOption::Mode::Auto;
        }

        return true;
    }

    return false;
}

void CommandLineOptions::PrintHelpBuildInfo()
{
    this->DoConsole(
    [&](IConsoleIo* console_io)->void
        {
            console_io->SetColor(ConsoleColor::LIGHT_YELLOW, ConsoleColor::DEFAULT);
            console_io->WriteLineStdOut("Build Information");
            console_io->WriteLineStdOut("-----------------");
            console_io->WriteLineStdOut("");
            console_io->SetColor(ConsoleColor::DARK_WHITE, ConsoleColor::DEFAULT);
            console_io->WriteStdOut("version          : ");
            console_io->SetColor(ConsoleColor::DARK_GREEN, ConsoleColor::DEFAULT);
            console_io->WriteLineStdOut(LIBPYRAMIDIZER_VERSION_MAJOR "." LIBPYRAMIDIZER_VERSION_MINOR "." LIBPYRAMIDIZER_VERSION_PATCH);

            console_io->SetColor(ConsoleColor::DARK_WHITE, ConsoleColor::DEFAULT);
            console_io->WriteStdOut("compiler         : ");
            console_io->SetColor(ConsoleColor::DARK_GREEN, ConsoleColor::DEFAULT);
            console_io->WriteLineStdOut(LIBPYRAMIDIZER_CXX_COMPILER_IDENTIFICATION);

            console_io->SetColor(ConsoleColor::DARK_WHITE, ConsoleColor::DEFAULT);
            console_io->WriteStdOut("repository-URL   : ");
            console_io->SetColor(ConsoleColor::DARK_GREEN, ConsoleColor::DEFAULT);
            console_io->WriteLineStdOut(LIBPYRAMIDIZER_REPOSITORYREMOTEURL);

            console_io->SetColor(ConsoleColor::DARK_WHITE, ConsoleColor::DEFAULT);
            console_io->WriteStdOut("repository-branch: ");
            console_io->SetColor(ConsoleColor::DARK_GREEN, ConsoleColor::DEFAULT);
            console_io->WriteLineStdOut(LIBPYRAMIDIZER_REPOSITORYBRANCH);

            console_io->SetColor(ConsoleColor::DARK_WHITE, ConsoleColor::DEFAULT);
            console_io->WriteStdOut("repository-tag   : ");
            console_io->SetColor(ConsoleColor::DARK_GREEN, ConsoleColor::DEFAULT);
            console_io->WriteLineStdOut(LIBPYRAMIDIZER_REPOSITORYHASH);

            console_io->SetColor(ConsoleColor::DEFAULT, ConsoleColor::DEFAULT);
        });
}

void CommandLineOptions::PrintHelpStreamsObjects()
{
    this->DoConsole(
        [](IConsoleIo* console_io)->void
        {
            console_io->SetColor(ConsoleColor::LIGHT_YELLOW, ConsoleColor::DEFAULT);
            console_io->WriteLineStdOut("Available Input-Stream objects");
            console_io->WriteLineStdOut("------------------------------");
            console_io->WriteLineStdOut("");
        });

    const int stream_object_count = libCZI::StreamsFactory::GetStreamClassesCount();

    for (int i = 0; i < stream_object_count; ++i)
    {
        libCZI::StreamsFactory::StreamClassInfo stream_class_info;
        libCZI::StreamsFactory::GetStreamInfoForClass(i, stream_class_info);

        this->DoConsole(
            [&](IConsoleIo* console_io)->void
            {
                console_io->SetColor(ConsoleColor::WHITE, ConsoleColor::DEFAULT);
                console_io->WriteStdOut(to_string(i + 1) + ": ");
                console_io->SetColor(ConsoleColor::DARK_GREEN, ConsoleColor::DEFAULT);
                console_io->WriteLineStdOut(stream_class_info.class_name);
                console_io->SetColor(ConsoleColor::DARK_WHITE, ConsoleColor::DEFAULT);
                console_io->WriteStdOut("    ");
                console_io->WriteLineStdOut(stream_class_info.short_description);
            });

        if (stream_class_info.get_build_info)
        {
            string build_info = stream_class_info.get_build_info();
            if (!build_info.empty())
            {
                this->DoConsole(
                    [&](IConsoleIo* console_io)->void
                    {
                        console_io->WriteStdOut("    ""Build: ");
                        console_io->WriteLineStdOut(build_info);
                    });
            }
        }
    }

    this->DoConsole(
        [&](IConsoleIo* console_io)->void
        {
            console_io->SetColor(ConsoleColor::DEFAULT, ConsoleColor::DEFAULT);
        });
}
