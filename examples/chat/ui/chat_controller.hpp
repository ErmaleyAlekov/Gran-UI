#ifndef CHAT_UI_CHAT_CONTROLLER_HPP
#define CHAT_UI_CHAT_CONTROLLER_HPP

#include "chat/chat_job.hpp"
#include "ui/chat_view.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace chat
{
    /*
    ** Контроллер чата (паттерн MVC: модель — история диалога, вид — ChatView). Живёт в потоке
    ** интерфейса; сетевая работа — в отдельном потоке (ChatJob), её события приходят обратно
    ** задачами Application::post (ReplyEvent) — дерево интерфейса трогает только этот поток.
    ** Один запрос за раз; у каждого свой номер, события устаревших запросов отбрасываются.
    **
    ** The chat controller (the MVC pattern: the model is the dialog history, the view is ChatView). It lives in the
    ** UI thread; the network work runs in a separate thread (ChatJob), its events come back
    ** as Application::post tasks (ReplyEvent) — only this thread touches the UI tree.
    ** One request at a time; each has its own number, the events of stale requests are dropped.
    */
    class ChatController
    {
    public:
        // ---- chat_controller.cpp ----

        // Действия send/stop регистрируются, проверка модели запускается сразу / The send/stop actions are registered, the model probe starts at once
        ChatController(gran::Application& application, const OllamaEndpoint& endpoint);

        // Отмена и ожидание сетевого потока: он не должен пережить приложение / Cancelling and waiting for the network thread: it must not outlive the application
        ~ChatController();
        ChatController(const ChatController&) = delete;
        ChatController& operator=(const ChatController&) = delete;

        // Отправить текст поля ввода (кнопка, Enter) / Send the input text (the button, Enter)
        void send();

        // Прервать ответ; уже пришедший текст остаётся в истории / Stop the reply; the already received text stays in the history
        void stop();

        // ---- chat_replies.cpp ----

        // Событие сетевого потока (в потоке интерфейса) / A network thread event (in the UI thread)
        void handle(const ReplyEvent& event);

    private:
        // ---- chat_controller.cpp ----

        // Запуск задачи в сетевом потоке (прежний поток дожидается) / Starting a job in the network thread (the former thread is awaited)
        void launch(JobKind kind);

        // ---- chat_replies.cpp ----

        // Ответ закончен: история, кнопки, строка состояния / The reply is over: the history, the buttons, the status line
        void finish(const std::string& status, StatusTone tone, bool keepReply);

        // Итог проверки модели при запуске / The model probe outcome at startup
        void probed(const ReplyEvent& event);

        gran::Application& _app;
        ChatView _view;
        OllamaEndpoint _endpoint;
        std::vector<ChatMessage> _history;
        // Text пузыря текущего ответа и уже пришедший текст / The current reply bubble Text and the already received text
        gran::ui::Node* _reply = nullptr;
        std::string _partial;
        unsigned _request = 0;
        bool _busy = false;
        // Поток последней задачи и её флаг отмены (общий с задачей) / The last job thread and its cancel flag (shared with the job)
        std::thread _worker;
        std::shared_ptr<std::atomic<bool>> _cancel;
    };
}

#endif
