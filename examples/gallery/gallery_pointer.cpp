#include "gallery_events.hpp"

#include <cstdio>
#include <memory>

namespace gallery
{
    namespace
    {
        // Захвачен ли указатель полем pad (общий флаг обработчиков) / Whether the pad area captured the pointer (a flag shared by the handlers)
        struct PadState
        {
            bool captured = false;
        };

        /*
        ** pointerDown захватывает указатель: pointerMove приходит и за краем поля (координаты local выходят
        ** за 0…размер), pointerUp снимает захват сам. Колесо пишет сдвиг и отменяет прокрутку.
        **
        ** pointerDown captures the pointer: pointerMove comes past the area edge too (the local coordinates go beyond
        ** 0…size), pointerUp removes the capture itself. The wheel writes the shift and cancels the scrolling.
        */
        struct PadPointer
        {
            gran::Application* application;
            std::shared_ptr<PadState> state;

            void operator()(gran::ui::Node& pad, gran::ui::UiEvent& event) const
            {
                char text[96];
                const char* suffix = "";

                if (event.type == gran::ui::EventType::POINTER_DOWN && event.button == gran::platform::MouseButton::LEFT)
                {
                    application->capturePointer(pad);
                    state->captured = true;
                }
                if (event.type == gran::ui::EventType::POINTER_UP)
                    state->captured = false;
                if (event.type == gran::ui::EventType::WHEEL)
                    std::snprintf(text, sizeof(text), "Колесо: %+.1f", static_cast<double>(event.wheel.y));
                if (state->captured)
                    suffix = " (захват)";
                if (event.type == gran::ui::EventType::POINTER_LEAVE && !state->captured)
                    std::snprintf(text, sizeof(text), "Указатель вне поля");
                else if (event.type != gran::ui::EventType::WHEEL)
                    std::snprintf(text, sizeof(text), "x = %.0f, y = %.0f%s", static_cast<double>(event.local.x),
                        static_cast<double>(event.local.y), suffix);
                if (event.type == gran::ui::EventType::WHEEL)
                    event.preventDefault();
                setText(*application, "pointerInfo", text);
            }
        };

        // Своё контекстное меню вместо встроенного: preventDefault и слой в точке щелчка / A custom context menu instead of the built-in one: preventDefault and the layer at the click point
        struct PadMenu
        {
            gran::Application* application;

            void operator()(gran::ui::Node&, gran::ui::UiEvent& event) const
            {
                gran::ui::Node* menu = application->find("padMenu");

                event.preventDefault();
                if (menu != nullptr)
                    application->showPopupAt(*menu, event.position);
            }
        };

        // Пункт меню поля / The area menu item
        struct PadClear
        {
            gran::Application* application;

            void operator()(gran::ui::Node&) const
            {
                setText(*application, "pointerInfo", "x = —, y = —");
            }
        };
    }

    // Одно состояние на все обработчики поля / One state for all the area handlers
    void registerPointer(gran::Application& application)
    {
        PadPointer pointer{&application, std::make_shared<PadState>()};

        application.onEvent("padDown", pointer);
        application.onEvent("padMove", pointer);
        application.onEvent("padUp", pointer);
        application.onEvent("padLeave", pointer);
        application.onEvent("padWheel", pointer);
        application.onEvent("padMenu", PadMenu{&application});
        application.onAction("padClear", PadClear{&application});
    }
}
