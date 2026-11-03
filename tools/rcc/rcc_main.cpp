/*
** gran_rcc — встраивает ресурсы программы (разметку, картинки, шрифты, данные) в exe.
** Вызов: gran_rcc <манифест.xml> <выход.cpp> [--blob <пакет.bin>] [--depfile <выход.d>].
** Манифест проверяется схемой gran-resources.xsd; выходные файлы перезаписываются, только если
** их содержимое изменилось (make не пересобирает лишнего).
**
** gran_rcc embeds the program resources (markup, images, fonts, data) into the exe.
** Usage: gran_rcc <manifest.xml> <output.cpp> [--blob <bundle.bin>] [--depfile <output.d>].
** The manifest is checked by the gran-resources.xsd schema; the output files are rewritten only when
** their content has changed (make does not rebuild anything needlessly).
*/
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/res/manifest.hpp"

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
    // Аргументы командной строки / The command line arguments
    struct Arguments
    {
        std::string manifest;
        std::string source;
        std::string blob;
        std::string depfile;
    };

    // Разбор аргументов; false — неверный вызов / Parsing the arguments; false — a wrong call
    bool parseArguments(int argc, char* argv[], Arguments& arguments)
    {
        if (argc < 3)
            return false;
        arguments.manifest = argv[1];
        arguments.source = argv[2];
        for (int index = 3; index + 1 < argc; index += 2)
        {
            if (std::strcmp(argv[index], "--blob") == 0)
                arguments.blob = argv[index + 1];
            else if (std::strcmp(argv[index], "--depfile") == 0)
                arguments.depfile = argv[index + 1];
            else
                return false;
        }
        return argc % 2 == 1;
    }

    // Записать, только если содержимое другое; ошибка записи — GranException(IO) / Write only if the content differs; a write error is GranException(IO)
    void writeIfChanged(const std::string& path, const std::string& content)
    {
        std::ifstream input(std::filesystem::u8path(path), std::ios::binary);
        std::ostringstream current;

        current << input.rdbuf();
        if (input && current.str() == content)
            return;
        input.close();
        std::ofstream output(std::filesystem::u8path(path), std::ios::binary | std::ios::trunc);
        output << content;
        if (!output)
            throw gran::GranException(gran::ErrorCode::IO, gran::i18n::format(gran::i18n::Message::IO_WRITE_FAILED, {path}));
    }

    // Манифест → .cpp, блок и файл зависимостей по запросу / The manifest → .cpp, the block and the dependency file on request
    void run(const Arguments& arguments)
    {
        gran::res::Manifest manifest = gran::res::readManifest(std::filesystem::u8path(arguments.manifest));

        writeIfChanged(arguments.source, gran::res::generateSource(manifest));
        if (!arguments.blob.empty())
            writeIfChanged(arguments.blob, gran::res::generateBlob(manifest));
        if (!arguments.depfile.empty())
            writeIfChanged(arguments.depfile, gran::res::generateDepfile(manifest, arguments.source));
    }
}

// Код 2 — неверный вызов, 1 — ошибка манифеста или файла / The code 2 — a wrong call, 1 — a manifest or file error
int main(int argc, char* argv[])
{
    Arguments arguments;

    if (!parseArguments(argc, argv, arguments))
    {
        std::cerr << gran::i18n::format(gran::i18n::Message::RES_TOOL_USAGE) << '\n';
        return 2;
    }
    try
    {
        run(arguments);
    }
    catch (const std::exception& error)
    {
        std::cerr << arguments.manifest << ": " << error.what() << '\n';
        return 1;
    }
    return 0;
}
