#include "gran/app/internal/app_state.hpp"

#include <utility>

namespace gran
{
    // Обработчик заменяет прежний с тем же именем / The handler replaces a former one with the same name
    void Application::onEvent(const std::string& name, EventHandler handler)
    {
        _state->eventHandlers[name] = std::move(handler);
    }

    // Встроенное имя даёт встроенный вид, прочее — своё событие / A built-in name gives the built-in kind, the rest is a custom event
    ListenerId Application::addEventListener(ui::Node& node, const std::string& type, EventHandler handler, bool capture)
    {
        ui::EventListener listener;

        listener.type = ui::eventType(type);
        listener.name = type;
        listener.capture = capture;
        listener.id = ++_state->lastListener;
        listener.handler = std::move(handler);
        node.listeners.push_back(std::move(listener));
        return node.listeners.back().id;
    }

    // Доставка вызывает копии обработчиков — снятие во время неё безопасно / The delivery calls handler copies — removing during it is safe
    void Application::removeEventListener(ui::Node& node, ListenerId id)
    {
        for (std::size_t index = 0; index < node.listeners.size(); ++index)
        {
            if (node.listeners[index].id == id)
            {
                node.listeners.erase(node.listeners.begin() + static_cast<std::ptrdiff_t>(index));
                return;
            }
        }
    }
}
