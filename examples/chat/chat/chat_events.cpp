#include "ui/chat_controller.hpp"
#include "ui/chat_actions.hpp"

namespace chat
{
    // Задача потока интерфейса: событие — контроллеру / The UI thread task: the event goes to the controller
    void ReplyEvent::operator()() const
    {
        controller->handle(*this);
    }

    // onClick="send" и onSubmit="send" (Enter в поле) / onClick="send" and onSubmit="send" (Enter in the input)
    void SendAction::operator()(gran::ui::Node& node) const
    {
        (void)node;
        controller->send();
    }

    // onClick="stop" / onClick="stop"
    void StopAction::operator()(gran::ui::Node& node) const
    {
        (void)node;
        controller->stop();
    }
}
