#ifndef CHAT_COMMON_CHAT_TEXTS_HPP
#define CHAT_COMMON_CHAT_TEXTS_HPP

#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>

namespace chat
{
    /*
    ** Идентификаторы текстов, которые видит пользователь чата. Тексты на каждом языке —
    ** в chat_texts_ru.cpp / chat_texts_en.cpp; язык берётся у движка (gran::i18n::language()).
    ** Новый текст добавляется перед COUNT и в оба каталога.
    **
    ** The identifiers of the texts the chat user sees. The texts in each language live in
    ** chat_texts_ru.cpp / chat_texts_en.cpp; the language comes from the engine (gran::i18n::language()).
    ** A new text is added before COUNT and to both catalogs.
    */
    enum class ChatText : std::uint16_t
    {
        // ---- Строка состояния / The status line ----
        STATUS_CONNECTING,
        STATUS_READY,
        STATUS_THINKING,
        STATUS_ANSWERING,
        STATUS_STOPPED,
        STATUS_OFFLINE,
        STATUS_MODEL_MISSING,
        STATUS_FAILED,

        // ---- Разбор JSON / JSON parsing ----
        JSON_UNEXPECTED_END,
        JSON_UNEXPECTED_CHAR,
        JSON_BAD_ESCAPE,
        JSON_BAD_NUMBER,
        JSON_TOO_DEEP,

        // ---- HTTP и сеть / HTTP and the network ----
        CHUNK_BAD_SIZE,
        CHUNK_BAD_END,
        LINE_TOO_LONG,
        HTTP_BAD_HEAD,
        HTTP_STATUS,
        NET_STARTUP,
        NET_CONNECT,
        NET_SEND,
        NET_RECEIVE,
        NET_CLOSED,

        // ---- Ollama / Ollama ----
        OLLAMA_ERROR,

        // ---- Программа / The program ----
        APP_FATAL,
        APP_MISSING_NODE,

        COUNT
    };

    // Запись каталога: идентификатор и шаблон с метками {0}, {1}... / A catalog entry: an identifier and a template with {0}, {1}... placeholders
    struct ChatTextEntry
    {
        ChatText id;
        const char* text;
    };

    // Каталог языка / A language catalog
    struct ChatCatalog
    {
        const char* code;
        const ChatTextEntry* entries;
        std::size_t size;
    };

    // Каталоги из chat_texts_ru.cpp и chat_texts_en.cpp / The catalogs from chat_texts_ru.cpp and chat_texts_en.cpp
    extern const ChatCatalog CHAT_RUSSIAN;
    extern const ChatCatalog CHAT_ENGLISH;

    // ---- chat_texts.cpp ----

    // Текст на языке движка с подставленными аргументами (без перевода — английский) / The text in the engine language with the arguments (without a translation — English)
    std::string say(ChatText id, std::initializer_list<std::string_view> arguments = {});

    // Шаблон из каталога или nullptr / A template from a catalog or nullptr
    const char* findChatText(const ChatCatalog& catalog, ChatText id) noexcept;
}

#endif
