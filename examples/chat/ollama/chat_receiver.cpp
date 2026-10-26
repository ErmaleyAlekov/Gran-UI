#include "ollama/chat_receiver.hpp"
#include "common/chat_errors.hpp"

#include <vector>

namespace chat
{
    // Слушатель живёт дольше приёмника (оба — на время одного запроса) / The listener outlives the receiver (both last for one request)
    ChatReceiver::ChatReceiver(ReplyListener& listener)
        : _listener(listener)
    {
    }

    // Код ответа запоминается до тела / The status code is remembered before the body
    void ChatReceiver::onHead(const HttpHead& received)
    {
        head = received;
    }

    // Не 200 — тело копится для ошибки; 200 — строки разбираются по мере прихода / Not 200 — the body is accumulated for the error; 200 — the lines are parsed as they arrive
    void ChatReceiver::onBody(std::string_view data)
    {
        std::vector<std::string> lines;

        if (head.status != 200)
        {
            failure.append(data);
            return;
        }
        _splitter.feed(data, lines);
        for (const std::string& line : lines)
            handleLine(line);
    }

    // Отмена — у слушателя / The cancel comes from the listener
    bool ChatReceiver::cancelled() const noexcept
    {
        return _listener.cancelled();
    }

    // Итоговая строка {"done":true} приходит без текста — слушатель её не видит / The final {"done":true} line comes without text — the listener does not see it
    void ChatReceiver::handleLine(const std::string& line)
    {
        ChatChunk chunk = parseChatLine(line);

        if (!chunk.error.empty())
            throw ChatException(ChatText::OLLAMA_ERROR, {chunk.error});
        if (!chunk.content.empty())
            _listener.onPiece(chunk.content);
    }
}
