#include "ui/chat_controller.hpp"
#include "common/chat_texts.hpp"

namespace chat
{
    /*
    ** События проверки модели принимаются всегда; события ответа — только текущего
    ** запроса и только пока он идёт (после «Остановить» поток ещё может что-то прислать).
    **
    ** The model probe events are always accepted; the reply events — only of the current
    ** request and only while it goes on (after "Stop" the thread may still send something).
    */
    void ChatController::handle(const ReplyEvent& event)
    {
        if (event.request == PROBE_REQUEST)
        {
            probed(event);
            return;
        }
        if (event.request != _request || !_busy)
            return;
        if (event.kind == ReplyKind::PIECE)
        {
            if (_partial.empty())
                _view.setStatus(say(ChatText::STATUS_ANSWERING), StatusTone::BUSY);
            _partial += event.text;
            _view.showReply(*_reply, _partial);
        }
        else if (event.kind == ReplyKind::DONE)
            finish(say(ChatText::STATUS_READY), StatusTone::READY, true);
        else
        {
            std::string message = say(ChatText::STATUS_FAILED, {event.text});
            if (event.kind == ReplyKind::OFFLINE)
                message = say(ChatText::STATUS_OFFLINE, {_endpoint.host + ":" + std::to_string(_endpoint.port)});
            _view.showError(*_reply, message);
            finish(message, StatusTone::FAILED, false);
        }
    }

    // Ответ в историю (или вопрос из неё — если ответа нет), кнопки и фокус — обратно к вводу / The reply into the history (or the question out of it — if there is no reply), the buttons and the focus back to the input
    void ChatController::finish(const std::string& status, StatusTone tone, bool keepReply)
    {
        if (keepReply)
            _history.push_back(ChatMessage{"assistant", _partial});
        else if (!_history.empty())
            _history.pop_back();
        _busy = false;
        _reply = nullptr;
        _partial.clear();
        _view.setBusy(false);
        _view.setStatus(status, tone);
    }

    // Готова / не установлена / Ollama не отвечает / Ready / not installed / Ollama does not answer
    void ChatController::probed(const ReplyEvent& event)
    {
        if (_busy)
            return;
        if (event.kind == ReplyKind::MODEL_READY)
            _view.setStatus(say(ChatText::STATUS_READY), StatusTone::READY);
        else if (event.kind == ReplyKind::MODEL_MISSING)
            _view.setStatus(say(ChatText::STATUS_MODEL_MISSING, {event.text}), StatusTone::FAILED);
        else if (event.kind == ReplyKind::OFFLINE)
            _view.setStatus(say(ChatText::STATUS_OFFLINE, {_endpoint.host + ":" + std::to_string(_endpoint.port)}),
                StatusTone::FAILED);
        else
            _view.setStatus(say(ChatText::STATUS_FAILED, {event.text}), StatusTone::FAILED);
    }
}
