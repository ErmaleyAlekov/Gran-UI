#include "chat/chat_job.hpp"
#include "common/chat_errors.hpp"

#include <exception>
#include <utility>

namespace chat
{
    namespace
    {
        // Событие → задача потока интерфейса / An event → a UI thread task
        void deliver(const ChatJob& job, ReplyKind kind, std::string text)
        {
            job.application->post(ReplyEvent{job.controller, job.request, kind, std::move(text)});
        }

        // Куски ответа уходят в поток интерфейса по одному (единицы-десятки в секунду) / The reply pieces go to the UI thread one by one (units to tens per second)
        class PostingListener final : public ReplyListener
        {
        public:
            explicit PostingListener(const ChatJob& job) : _job(job) {}

            // Кусок → задача / A piece → a task
            void onPiece(const std::string& text) override
            {
                deliver(_job, ReplyKind::PIECE, text);
            }

            // Флаг, выставленный кнопкой «Остановить» или деструктором контроллера / The flag set by the "Stop" button or the controller destructor
            bool cancelled() const noexcept override
            {
                return _job.cancel->load();
            }

        private:
            const ChatJob& _job;
        };
    }

    /*
    ** Ни одно исключение не покидает поток (иначе std::terminate): сбой сети — OFFLINE
    ** (строка состояния подскажет запустить Ollama), прочее — FAILED с текстом ошибки.
    ** Прерванный ответ ничего не сообщает: контроллер уже показал «остановлено».
    **
    ** No exception leaves the thread (otherwise std::terminate): a network failure is OFFLINE
    ** (the status line suggests starting Ollama), the rest is FAILED with the error text.
    ** A stopped reply reports nothing: the controller has shown "stopped" already.
    */
    void ChatJob::operator()() const
    {
        try
        {
            if (kind == JobKind::PROBE)
            {
                ReplyKind outcome = ReplyKind::MODEL_MISSING;
                if (probeModel(endpoint))
                    outcome = ReplyKind::MODEL_READY;
                deliver(*this, outcome, endpoint.model);
                return;
            }
            PostingListener listener(*this);
            if (streamChat(endpoint, history, listener))
                deliver(*this, ReplyKind::DONE, {});
        }
        catch (const NetworkException& error)
        {
            deliver(*this, ReplyKind::OFFLINE, error.what());
        }
        catch (const std::exception& error)
        {
            deliver(*this, ReplyKind::FAILED, error.what());
        }
    }
}
