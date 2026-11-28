#include "ui/controller.hpp"
#include "gran/utils/program_arguments.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    // Разметка ищется в рабочей папке и рядом с программой.
    // Look for markup in the working directory and beside the executable.
    std::string defaultMarkup(const char* program)
    {
        std::filesystem::path local = "samples/editor.xml";
        auto beside = std::filesystem::u8path(program).parent_path() / ".." / local;
        if (!std::filesystem::exists(local) && std::filesystem::exists(beside))
            return beside.lexically_normal().u8string();
        return local.u8string();
    }

    // XML как позиционный аргумент открывается как документ; разметка — через --markup.
    // Positional XML files open as documents; use --markup for the layout.
    gran::AppOptions parseOptions(int argc, char* argv[], std::string& markup, std::string& file)
    {
        gran::AppOptions options;
        options.appName = "Gran UI Editor";
        for (int index = 1; index < argc; ++index)
        {
            std::string argument = argv[index];
            if (argument == "--debug") options.debugLayer = true;
            else if (argument == "--no-overlay") options.skipCustomization = true;
            else if (argument == "--d3d11") options.backend = gran::rhi::BackendKind::D3D11;
            else if (argument == "--d3d12") options.backend = gran::rhi::BackendKind::D3D12;
            else if (argument == "--vulkan") options.backend = gran::rhi::BackendKind::VULKAN;
            else if (argument == "--software" || argument == "--warp") options.forceWarp = true;
            else if (argument == "--exit-after" && index + 1 < argc) options.exitAfter = std::strtod(argv[++index], nullptr);
            else if (argument == "--markup" && index + 1 < argc) markup = argv[++index];
            else if (file.empty()) file = argument;
        }
        return options;
    }
}

// Контроллер уничтожается только после завершения цикла приложения.
// The controller is destroyed only after the application loop finishes.
int main(int argc, char* argv[])
{
    // Аргументы — в UTF-8: u8path ниже ждёт именно его (на Windows argv в ANSI) / The arguments are in UTF-8: u8path below expects it (ANSI argv on Windows)
    std::vector<std::string> arguments = gran::programArguments(argc, argv);
    std::vector<char*> utf8;

    for (std::string& argument : arguments)
        utf8.push_back(&argument[0]);
    argc = static_cast<int>(utf8.size());
    argv = utf8.data();
    std::string markup;
    std::string file;
    auto options = parseOptions(argc, argv, markup, file);
    if (markup.empty()) markup = defaultMarkup(arguments.empty() ? "" : argv[0]);
    try
    {
        gran::Application app(options);
        app.loadFile(markup);
        editor::Controller controller(app);
        std::string workspace;
        if (file.empty())
            workspace = std::filesystem::absolute(std::filesystem::u8path(markup)).parent_path().parent_path().u8string();
        controller.start(file, workspace);
        return app.run();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
