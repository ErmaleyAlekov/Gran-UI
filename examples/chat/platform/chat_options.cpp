#include "platform/chat_options.hpp"
#include "common/chat_errors.hpp"
#include <charconv>
#include <cstdlib>
#include <filesystem>

namespace chat
{
    // Выбор API не зависит от сетевых параметров чата.
    // API selection is independent of the chat network options.
    static bool graphicsOption(const std::string& argument, gran::AppOptions& options)
    {
        if (argument == "--d3d11") options.backend = gran::rhi::BackendKind::D3D11;
        else if (argument == "--d3d12") options.backend = gran::rhi::BackendKind::D3D12;
        else if (argument == "--warp") options.forceWarp = true;
        else if (argument == "--debug") options.debugLayer = true;
        else return false;
        return true;
    }

    /*
    ** Разметка по умолчанию: от текущей папки (запуск из корня проекта), затем от папки
    ** программы (двойной щелчок по bin/gran_chat.exe — текущая папка тогда bin/).
    **
    ** The default markup: from the current folder (a run from the project root), then from the program
    ** folder (a double click on bin/gran_chat.exe — the current folder is bin/ then).
    */
    static std::string defaultMarkup(const std::string& program)
    {
        std::filesystem::path local = "examples/chat/chat.xml";
        std::filesystem::path besideProgram = std::filesystem::path(program).parent_path() / ".." / local;

        if (!std::filesystem::exists(local) && std::filesystem::exists(besideProgram))
            return besideProgram.lexically_normal().string();
        return local.string();
    }

    // Десятичный номер TCP-порта. / Decimal TCP port number.
    std::uint16_t parsePort(const std::string& text)
    {
        std::uint16_t port = 0;
        auto parsed = std::from_chars(text.data(), text.data() + text.size(), port);
        if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || port == 0)
            throw ChatException(ChatText::APP_BAD_PORT, {});
        return port;
    }

    // Ключи с значением берут следующий аргумент; остальное — путь к разметке (аргументы уже в UTF-8) / The keys with a value take the next argument; the rest is the markup path (the arguments are UTF-8 already)
    ChatOptions parseOptions(const std::vector<std::string>& argv)
    {
        ChatOptions options;

        if (!argv.empty())
            options.markup = defaultMarkup(argv[0]);
        for (std::size_t index = 1; index < argv.size(); ++index)
        {
            const auto& option = argv[index];
            if (graphicsOption(option, options.app))
                continue;
            if (index + 1 == argv.size() && (option == "--exit-after" || option == "--model"
                || option == "--host" || option == "--port" || option == "--ask"))
                throw ChatException(ChatText::APP_MISSING_VALUE, {option});
            if (option == "--exit-after")
                options.app.exitAfter = std::strtod(argv[++index].c_str(), nullptr);
            else if (option == "--model")
                options.endpoint.model = argv[++index];
            else if (option == "--host")
                options.endpoint.host = argv[++index];
            else if (option == "--port")
                options.endpoint.port = parsePort(argv[++index]);
            else if (option == "--ask")
                options.ask = argv[++index];
            else
                options.markup = argv[index];
        }
        return options;
    }
}
