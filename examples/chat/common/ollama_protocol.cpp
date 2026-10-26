#include "common/ollama_protocol.hpp"
#include "common/json_value.hpp"

namespace chat
{
    namespace
    {
        // Совпадение имён моделей с учётом тега :latest по умолчанию / Matching the model names considering the default :latest tag
        bool sameModel(const std::string& installed, const std::string& wanted)
        {
            if (installed == wanted)
                return true;
            if (wanted.find(':') == std::string::npos)
                return installed == wanted + ":latest";
            return false;
        }
    }

    // JSON собирается вручную: структура фиксирована, экранирование — writeJsonString / The JSON is assembled by hand: the structure is fixed, the escaping is writeJsonString
    std::string buildChatRequest(const std::string& model, const std::vector<ChatMessage>& history)
    {
        std::string body = "{\"model\":";

        writeJsonString(body, model);
        body += ",\"stream\":true,\"messages\":[";
        for (std::size_t index = 0; index < history.size(); ++index)
        {
            if (index > 0)
                body += ',';
            body += "{\"role\":";
            writeJsonString(body, history[index].role);
            body += ",\"content\":";
            writeJsonString(body, history[index].content);
            body += '}';
        }
        body += "]}";
        return body;
    }

    // Отсутствующие поля — пустые (итоговая строка приходит без message или с пустым) / Missing fields are empty (the final line comes without message or with an empty one)
    ChatChunk parseChatLine(std::string_view line)
    {
        JsonValue root = parseJson(line);
        ChatChunk chunk;
        const JsonValue* message = jsonMember(root, "message");

        if (message != nullptr)
            chunk.content = jsonString(*message, "content");
        chunk.error = jsonString(root, "error");
        chunk.done = jsonBoolean(root, "done");
        return chunk;
    }

    // Поиск по массиву models / A search over the models array
    bool modelInstalled(std::string_view tagsJson, const std::string& model)
    {
        JsonValue root = parseJson(tagsJson);
        const JsonValue* models = jsonMember(root, "models");

        if (models == nullptr || models->type != JsonType::ARRAY)
            return false;
        for (const JsonValue& entry : models->items)
        {
            if (sameModel(jsonString(entry, "name"), model) || sameModel(jsonString(entry, "model"), model))
                return true;
        }
        return false;
    }
}
