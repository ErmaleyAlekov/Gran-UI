#ifndef CHAT_COMMON_OLLAMA_PROTOCOL_HPP
#define CHAT_COMMON_OLLAMA_PROTOCOL_HPP

#include <string>
#include <string_view>
#include <vector>

namespace chat
{
    // Реплика диалога: роль ("user", "assistant", "system") и текст / A dialog turn: the role ("user", "assistant", "system") and the text
    struct ChatMessage
    {
        std::string role;
        std::string content;
    };

    // Одна строка потокового ответа /api/chat / One line of the streaming /api/chat reply
    struct ChatChunk
    {
        std::string content;
        std::string error;
        bool done = false;
    };

    /*
    ** Тело POST /api/chat: {"model":…,"stream":true,"messages":[{"role":…,"content":…},…]} —
    ** история диалога целиком (у модели нет памяти между запросами).
    **
    ** The POST /api/chat body: {"model":…,"stream":true,"messages":[{"role":…,"content":…},…]} —
    ** the whole dialog history (the model has no memory between requests).
    */
    std::string buildChatRequest(const std::string& model, const std::vector<ChatMessage>& history);

    /*
    ** Строка NDJSON ответа: {"message":{"content":"кусок"},"done":false} или итоговая
    ** {"done":true,…}; ошибка сервера — {"error":"…"}. Неверный JSON — JsonException.
    **
    ** An NDJSON reply line: {"message":{"content":"piece"},"done":false} or the final
    ** {"done":true,…}; a server error is {"error":"…"}. Invalid JSON — JsonException.
    */
    ChatChunk parseChatLine(std::string_view line);

    /*
    ** Ответ GET /api/tags: {"models":[{"name":"qwen2.5-coder:14b",…},…]}. Имя без тега
    ** совпадает с «имя:latest», как в самой Ollama.
    **
    ** The GET /api/tags reply: {"models":[{"name":"qwen2.5-coder:14b",…},…]}. A name without a tag
    ** matches "name:latest", as in Ollama itself.
    */
    bool modelInstalled(std::string_view tagsJson, const std::string& model);
}

#endif
