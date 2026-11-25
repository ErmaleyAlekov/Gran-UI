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

#include "platform/chat_options.hpp"
#include <string>
#include <vector>

namespace
{
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
    try
    {
        auto options = chat::parseOptions(chat::utf8Arguments());
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
