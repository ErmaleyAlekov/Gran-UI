#include "ui/chat_controller.hpp"
#include "ui/chat_actions.hpp"
#include "common/chat_texts.hpp"

#include <utility>

namespace chat
{
    // Вид строится сразу (ошибка разметки — до окна), затем проверка модели в фоне / The view is built at once (a markup error — before the window), then the model probe in the background
    ChatController::ChatController(gran::Application& application, const OllamaEndpoint& endpoint)
        : _app(application), _view(application, endpoint.model), _endpoint(endpoint)
    {
        _app.onAction("send", SendAction{this});
        _app.onAction("stop", StopAction{this});
        _view.setStatus(say(ChatText::STATUS_CONNECTING), StatusTone::BUSY);
        launch(JobKind::PROBE);
    }

    // Флаг отмены будит поток не позже чем через CANCEL_POLL_MS / The cancel flag wakes the thread within CANCEL_POLL_MS
    ChatController::~ChatController()
    {
        if (_cancel)
            _cancel->store(true);
        if (_worker.joinable())
            _worker.join();
    }

    // Пустой текст и повторная отправка во время ответа игнорируются / An empty text and a repeated send during a reply are ignored
    void ChatController::send()
    {
        if (_busy)
            return;
        std::string text = _view.takePrompt();
        if (text.empty())
            return;
        _history.push_back(ChatMessage{"user", text});
        _view.addBubble(BubbleRole::USER, text);
        _reply = &_view.addBubble(BubbleRole::MODEL, {});
        _partial.clear();
        _busy = true;
        ++_request;
        _view.setBusy(true);
        _view.setStatus(say(ChatText::STATUS_THINKING), StatusTone::BUSY);
        launch(JobKind::CHAT);
    }

    // Поток узнает об отмене сам; его поздние события отбросятся (_busy уже false) / The thread learns about the cancel itself; its late events are dropped (_busy is false already)
    void ChatController::stop()
    {
        if (!_busy)
            return;
        _cancel->store(true);
        if (_partial.empty())
            _view.showError(*_reply, say(ChatText::STATUS_STOPPED));
        finish(say(ChatText::STATUS_STOPPED), StatusTone::IDLE, !_partial.empty());
    }

    /*
    ** Прежний поток к этому моменту завершён или отменён (кнопка «Отправить» скрыта, пока идёт
    ** ответ), поэтому join не задерживает интерфейс дольше одного периода опроса отмены.
    **
    ** The former thread has finished or been cancelled by now (the "Send" button is hidden while a reply
    ** goes on), so join does not delay the interface longer than one cancel polling period.
    */
    void ChatController::launch(JobKind kind)
    {
        unsigned request = PROBE_REQUEST;

        if (_worker.joinable())
            _worker.join();
        if (kind == JobKind::CHAT)
            request = _request;
        _cancel = std::make_shared<std::atomic<bool>>(false);
        _worker = std::thread(ChatJob{&_app, this, request, kind, _endpoint, _history, _cancel});
    }
}
