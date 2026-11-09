/*
** gran_gallery — интерактивная витрина: разметка samples/showcase.xml в окне с действиями.
** Вызов: gran_gallery [разметка.xml] [--debug] [--exit-after секунды]
**
** gran_gallery — an interactive showcase: the samples/showcase.xml markup in a window with actions.
** Usage: gran_gallery [markup.xml] [--debug] [--exit-after seconds]
*/
#include "gran/res/resources.hpp"
#include "gallery_actions.hpp"
#include "gran/core/gran_exception.hpp"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>

namespace
{
    /*
    ** Витрина по умолчанию: сначала от текущей папки (запуск из корня проекта), затем от папки
    ** программы (двойной щелчок по bin/gran_gallery.exe — текущая папка тогда bin/).
    **
    ** The default showcase: first from the current folder (a run from the project root), then from the program
    ** folder (a double click on bin/gran_gallery.exe — the current folder is bin/ then).
    */
    std::string defaultMarkup(const char* program)
    {
        std::filesystem::path local = "samples/showcase.xml";
        std::filesystem::path besideProgram = std::filesystem::path(program).parent_path() / ".." / local;

        if (!std::filesystem::exists(local) && std::filesystem::exists(besideProgram))
            return besideProgram.lexically_normal().string();
        return local.string();
    }

    /*
    ** Аргумент, который не стал ключом: разметка, если не похож на ключ. Похожий на ключ (начинается с дефиса
    ** или с длинного тире, которое подставляют текстовые редакторы и просмотрщики Markdown вместо «--»)
    ** не считается файлом, иначе программа молча откроет файл «--vulkan» и упадёт с непонятной ошибкой.
    **
    ** An argument that did not become a key: the markup, unless it looks like a key. One that looks like a key
    ** (starts with a hyphen or with the en/em dash that text editors and Markdown viewers put instead of "--")
    ** is not taken as a file, otherwise the program silently opens a "--vulkan" file and fails with an obscure error.
    */
    void acceptMarkup(const char* argument, std::string& markup)
    {
        // Длинное тире U+2013 и U+2014 в UTF-8 / The en dash U+2013 and the em dash U+2014 in UTF-8
        const bool dash = std::strncmp(argument, "\xE2\x80\x93", 3) == 0 || std::strncmp(argument, "\xE2\x80\x94", 3) == 0;

        if (argument[0] != '-' && !dash)
        {
            markup = argument;
            return;
        }
        std::cerr << "gran_gallery: неизвестный ключ \"" << argument << "\" (ключи пишутся двумя дефисами: --vulkan)\n"
                  << "gran_gallery: unknown key \"" << argument << "\" (keys are written with two hyphens: --vulkan)\n";
    }

    // Ключи командной строки / The command line keys
    gran::AppOptions parseOptions(int argc, char* argv[], std::string& markup)
    {
        gran::AppOptions options;

        // Журнал и отчёт о сбое галереи: %LOCALAPPDATA%\GranGallery (Windows), ~/.local/state/GranGallery (Linux)
        // The gallery log and crash report: %LOCALAPPDATA%\GranGallery (Windows), ~/.local/state/GranGallery (Linux)
        options.appName = "GranGallery";

        for (int index = 1; index < argc; ++index)
        {
            if (std::strcmp(argv[index], "--debug") == 0)
                options.debugLayer = true;
            else if (std::strcmp(argv[index], "--watch") == 0)
                options.watchMarkup = true;
            else if (std::strcmp(argv[index], "--d3d11") == 0)
                options.backend = gran::rhi::BackendKind::D3D11;
            else if (std::strcmp(argv[index], "--d3d12") == 0)
                options.backend = gran::rhi::BackendKind::D3D12;
            else if (std::strcmp(argv[index], "--vulkan") == 0)
                options.backend = gran::rhi::BackendKind::VULKAN;
            else if (std::strcmp(argv[index], "--software") == 0)
                options.forceWarp = true;
            else if (std::strcmp(argv[index], "--warp") == 0)
                options.forceWarp = true;
            else if (std::strcmp(argv[index], "--exit-after") == 0 && index + 1 < argc)
                options.exitAfter = std::strtod(argv[++index], nullptr);
            else
                acceptMarkup(argv[index], markup);
        }
        return options;
    }
}

// Точка входа: разметка, действия, цикл окна / The entry point: the markup, the actions, the window loop
int main(int argc, char* argv[])
{
    std::string markup = defaultMarkup(argv[0]);

    try
    {
        gran::Application application(parseOptions(argc, argv, markup));
        // Встроенные примеры (examples/gallery/resources.xml): gran_gallery res:/samples/program.xml
        // The embedded samples (examples/gallery/resources.xml): gran_gallery res:/samples/program.xml
        gran::res::mount(granResources_gallery());
        application.loadFile(markup);
        gallery::registerActions(application);
        gallery::registerTimers(application);
        gallery::registerPlatform(application, markup);
        gallery::registerEvents(application);
        gallery::registerProgram(application, markup);
        return application.run();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
