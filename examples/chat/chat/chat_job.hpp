#ifndef CHAT_CHAT_CHAT_JOB_HPP
#define CHAT_CHAT_CHAT_JOB_HPP

#include "gran/app/application.hpp"
#include "ollama/ollama_client.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace chat
{
    class ChatController;

    // Что делает сетевая задача / What the network job does
    enum class JobKind : std::uint8_t { PROBE, CHAT };

    // Вид события для потока интерфейса / The event kind for the UI thread
    enum class ReplyKind : std::uint8_t { PIECE, DONE, FAILED, OFFLINE, MODEL_READY, MODEL_MISSING };

    // Номер запроса проверки модели (запросы чата нумеруются с 1) / The model probe request number (the chat requests are numbered from 1)
    constexpr unsigned PROBE_REQUEST = 0;

    /*
    ** Событие сетевого потока, доставляемое в поток интерфейса через Application::post
    ** (паттерн Command: функтор вместо лямбды несёт всё нужное с собой).
    **
    ** A network thread event delivered to the UI thread through Application::post
    ** (the Command pattern: a functor instead of a lambda carries everything it needs).
    */
    struct ReplyEvent
    {
        ChatController* controller;
        unsigned request;
        ReplyKind kind;
        std::string text;

        // ---- chat_events.cpp ----
        void operator()() const;
    };

    /*
    ** Работа сетевого потока (функтор для std::thread): проверка модели или потоковый ответ.
    ** Всё нужное скопировано внутрь (история — копия), с потоком интерфейса связь только
    ** через post и общий флаг отмены.
    **
    ** The network thread work (a functor for std::thread): the model probe or a streaming reply.
    ** Everything needed is copied inside (the history is a copy), the link to the UI thread is only
    ** post and the shared cancel flag.
    */
    struct ChatJob
    {
        gran::Application* application;
        ChatController* controller;
        unsigned request;
        JobKind kind;
        OllamaEndpoint endpoint;
        std::vector<ChatMessage> history;
        std::shared_ptr<std::atomic<bool>> cancel;

        // ---- chat_job.cpp ----
        void operator()() const;
    };
}

#endif
