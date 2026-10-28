/*
** gran_chat — чат с локальной моделью Ollama на движке Gran UI (Win32 + D3D12).
** Вызов: gran_chat [разметка.xml] [--model имя] [--host адрес] [--port порт] [--ask "вопрос"]
**                  [--debug] [--exit-after секунды]
**
** gran_chat — a chat with a local Ollama model on the Gran UI engine (Win32 + D3D12).
** Usage: gran_chat [markup.xml] [--model name] [--host address] [--port port] [--ask "question"]
**                  [--debug] [--exit-after seconds]
*/
#include "common/chat_texts.hpp"
#include "net/tcp_connection.hpp"
#include "platform/fatal_error.hpp"
#include "platform/utf8_arguments.hpp"
#include "ui/chat_controller.hpp"

#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace
{
    // Выбор API не зависит от сетевых параметров чата.
    // API selection is independent of the chat network options.
    bool graphicsOption(const std::string& argument, gran::AppOptions& options)
    {
        if (argument == "--d3d11") options.backend = gran::rhi::BackendKind::D3D11;
        else if (argument == "--d3d12") options.backend = gran::rhi::BackendKind::D3D12;
        else if (argument == "--warp") options.forceWarp = true;
        else if (argument == "--debug") options.debugLayer = true;
        else return false;
        return true;
    }

    // Параметры командной строки / The command line parameters
    struct ChatOptions
    {
        gran::AppOptions app;
        chat::OllamaEndpoint endpoint;
        std::string markup;
        std::string ask;
    };

    /*
    ** Разметка по умолчанию: от текущей папки (запуск из корня проекта), затем от папки
    ** программы (двойной щелчок по bin/gran_chat.exe — текущая папка тогда bin/).
    **
    ** The default markup: from the current folder (a run from the project root), then from the program
    ** folder (a double click on bin/gran_chat.exe — the current folder is bin/ then).
    */
    std::string defaultMarkup(const std::string& program)
    {
        std::filesystem::path local = "examples/chat/chat.xml";
        std::filesystem::path besideProgram = std::filesystem::path(program).parent_path() / ".." / local;

        if (!std::filesystem::exists(local) && std::filesystem::exists(besideProgram))
            return besideProgram.lexically_normal().string();
        return local.string();
    }

    // Ключи с значением берут следующий аргумент; остальное — путь к разметке (аргументы уже в UTF-8) / The keys with a value take the next argument; the rest is the markup path (the arguments are UTF-8 already)
    ChatOptions parseOptions(const std::vector<std::string>& argv)
    {
        ChatOptions options;

        if (!argv.empty())
            options.markup = defaultMarkup(argv[0]);
        for (std::size_t index = 1; index < argv.size(); ++index)
        {
            bool hasValue = index + 1 < argv.size();
            if (graphicsOption(argv[index], options.app))
                continue;
            if (argv[index] == "--exit-after" && hasValue)
                options.app.exitAfter = std::strtod(argv[++index].c_str(), nullptr);
            else if (argv[index] == "--model" && hasValue)
                options.endpoint.model = argv[++index];
            else if (argv[index] == "--host" && hasValue)
                options.endpoint.host = argv[++index];
            else if (argv[index] == "--port" && hasValue)
                options.endpoint.port = static_cast<std::uint16_t>(std::strtoul(argv[++index].c_str(), nullptr, 10));
            else if (argv[index] == "--ask" && hasValue)
                options.ask = argv[++index];
            else
                options.markup = argv[index];
        }
        return options;
    }

    // --ask: вопрос в поле ввода и отправка до показа окна (демонстрация и проверка без мыши) / --ask: the question into the input and sending before the window shows (a demo and a check without the mouse)
    void askAtStart(gran::Application& application, chat::ChatController& controller, const std::string& question)
    {
        gran::ui::Node* prompt = application.find("prompt");

        if (question.empty() || prompt == nullptr)
            return;
        prompt->content.text = question;
        controller.send();
    }
}

/*
** Порядок объектов важен: контроллер объявлен после приложения и уничтожается раньше —
** его деструктор дожидается сетевого потока, пока приложение (получатель post) ещё живо;
** WinSock живёт дольше всех соединений.
**
** The object order matters: the controller is declared after the application and destroyed earlier —
** its destructor waits for the network thread while the application (the post receiver) is still alive;
** WinSock outlives all connections.
*/
int main()
{
    ChatOptions options = parseOptions(chat::utf8Arguments());

    try
    {
        chat::WinsockSession winsock;
        gran::Application application(options.app);
        application.loadFile(options.markup);
        chat::ChatController controller(application, options.endpoint);
        askAtStart(application, controller, options.ask);
        return application.run();
    }
    catch (const std::exception& error)
    {
        chat::showFatalError(chat::say(chat::ChatText::APP_FATAL, {error.what()}));
        return 1;
    }
}
