#ifndef CHAT_UI_CHAT_ACTIONS_HPP
#define CHAT_UI_CHAT_ACTIONS_HPP

#include "gran/ui/node.hpp"

namespace chat
{
    class ChatController;

    // onClick="send" и onSubmit="send" (функтор вместо лямбды) / onClick="send" and onSubmit="send" (a functor instead of a lambda)
    struct SendAction
    {
        ChatController* controller;

        // ---- chat/chat_events.cpp ----
        void operator()(gran::ui::Node& node) const;
    };

    // onClick="stop" / onClick="stop"
    struct StopAction
    {
        ChatController* controller;

        // ---- chat/chat_events.cpp ----
        void operator()(gran::ui::Node& node) const;
    };
}

#endif
