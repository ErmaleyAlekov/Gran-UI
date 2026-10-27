/*
** gran_gallery — интерактивная витрина: разметка samples/showcase.xml в окне с действиями.
** Вызов: gran_gallery [разметка.xml] [--debug] [--exit-after секунды]
**
** gran_gallery — an interactive showcase: the samples/showcase.xml markup in a window with actions.
** Usage: gran_gallery [markup.xml] [--debug] [--exit-after seconds]
*/
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

    // Ключи командной строки / The command line keys
    gran::AppOptions parseOptions(int argc, char* argv[], std::string& markup)
    {
        gran::AppOptions options;

        for (int index = 1; index < argc; ++index)
        {
            if (std::strcmp(argv[index], "--debug") == 0)
                options.debugLayer = true;
            else if (std::strcmp(argv[index], "--exit-after") == 0 && index + 1 < argc)
                options.exitAfter = std::strtod(argv[++index], nullptr);
            else
                markup = argv[index];
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
        application.loadFile(markup);
        gallery::registerActions(application);
        return application.run();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
