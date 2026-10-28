#include "gran/app/internal/app_state.hpp"

#include <utility>

namespace gran::app
{
    namespace
    {
        // Лежит ли узел в поддереве / Whether the node lies in the subtree
        bool inside(const ui::Node* node, const ui::Node& subtree) noexcept
        {
            for (const ui::Node* current = node; current != nullptr; current = current->parent)
            {
                if (current == &subtree)
                    return true;
            }
            return false;
        }
    }

    // Слотов столько, сколько WebView на экране, — линейного поиска достаточно / There are as many slots as WebViews on screen — a linear search is enough
    WebSlot& webSlot(AppWindow& win, ui::Node& node)
    {
        for (const std::unique_ptr<WebSlot>& slot : win.webSlots)
        {
            if (slot->node == &node)
                return *slot;
        }
        win.webSlots.push_back(std::make_unique<WebSlot>(win, node));
        return *win.webSlots.back();
    }

    // Уничтожение слота закрывает браузер и его окно-рамку / Destroying a slot closes the browser and its frame window
    void forgetWebViews(AppWindow& win, const ui::Node& subtree) noexcept
    {
        std::vector<std::unique_ptr<WebSlot>>& slots = win.webSlots;
        std::vector<WebMessage>& inbox = win.shared.webInbox;

        for (std::size_t index = 0; index < slots.size();)
        {
            if (inside(slots[index]->node, subtree))
                slots.erase(slots.begin() + static_cast<std::ptrdiff_t>(index));
            else
                ++index;
        }
        for (std::size_t index = 0; index < inbox.size();)
        {
            if (inside(inbox[index].node, subtree))
                inbox.erase(inbox.begin() + static_cast<std::ptrdiff_t>(index));
            else
                ++index;
        }
    }

    /*
    ** По одному сообщению из начала очереди: обработчик может удалить узел, и forgetWebViews
    ** уберёт его оставшиеся сообщения — локальная копия очереди держала бы висячие указатели.
    **
    ** One message at a time from the queue front: a handler may remove a node, and forgetWebViews
    ** drops its remaining messages — a local copy of the queue would hold dangling pointers.
    */
    bool deliverWebMessages(Application::State& state)
    {
        bool delivered = false;

        while (!state.webInbox.empty())
        {
            WebMessage message = std::move(state.webInbox.front());
            state.webInbox.erase(state.webInbox.begin());
            auto handler = state.webHandlers.find(message.node->content.onMessage);
            if (handler != state.webHandlers.end() && handler->second)
            {
                WebMessageHandler callback = handler->second;
                callback(*message.node, message.text);
            }
            delivered = true;
        }
        return delivered;
    }
}
