#include "common/chat_texts.hpp"

namespace chat
{
    namespace
    {
        // Русские тексты; {N} — аргументы say() / Russian texts; {N} are the say() arguments
        const ChatTextEntry TEXTS[] = {
            {ChatText::STATUS_CONNECTING, "подключение к Ollama…"},
            {ChatText::STATUS_READY, "готова"},
            {ChatText::STATUS_THINKING, "думает…"},
            {ChatText::STATUS_ANSWERING, "отвечает…"},
            {ChatText::STATUS_STOPPED, "ответ остановлен"},
            {ChatText::STATUS_OFFLINE, "Ollama не отвечает на {0} — запустите «ollama serve»"},
            {ChatText::STATUS_MODEL_MISSING, "модель {0} не установлена — «ollama pull {0}»"},
            {ChatText::STATUS_FAILED, "ошибка: {0}"},
            {ChatText::JSON_UNEXPECTED_END, "JSON оборван (позиция {0})"},
            {ChatText::JSON_UNEXPECTED_CHAR, "в JSON неожиданный символ «{0}» (позиция {1})"},
            {ChatText::JSON_BAD_ESCAPE, "в строке JSON неверная escape-последовательность (позиция {0})"},
            {ChatText::JSON_BAD_NUMBER, "в JSON неверное число (позиция {0})"},
            {ChatText::JSON_TOO_DEEP, "JSON вложен глубже {0} уровней"},
            {ChatText::CHUNK_BAD_SIZE, "неверный размер блока chunked: «{0}»"},
            {ChatText::CHUNK_BAD_END, "блок chunked не завершён переводом строки"},
            {ChatText::LINE_TOO_LONG, "строка ответа длиннее {0} байт"},
            {ChatText::HTTP_BAD_HEAD, "неверный заголовок ответа HTTP"},
            {ChatText::HTTP_STATUS, "сервер ответил {0} {1}"},
            {ChatText::NET_STARTUP, "не удалось запустить WinSock (код {0})"},
            {ChatText::NET_CONNECT, "не удалось подключиться к {0}:{1}"},
            {ChatText::NET_SEND, "не удалось отправить запрос (код {0})"},
            {ChatText::NET_RECEIVE, "ошибка приёма ответа (код {0})"},
            {ChatText::NET_CLOSED, "сервер закрыл соединение до конца ответа"},
            {ChatText::OLLAMA_ERROR, "Ollama: {0}"},
            {ChatText::APP_FATAL, "Gran Chat: {0}"},
            {ChatText::APP_MISSING_NODE, "в разметке чата нет элемента id=\"{0}\""}
        };
    }

    // Каталог русского языка / The Russian language catalog
    const ChatCatalog CHAT_RUSSIAN = {"ru", TEXTS, sizeof(TEXTS) / sizeof(TEXTS[0])};
}
