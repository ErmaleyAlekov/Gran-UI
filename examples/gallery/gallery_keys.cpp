#include "gallery_events.hpp"

namespace gallery
{
    namespace
    {
        // Фаза захвата у доски видит каждую клавишу раньше поля в фокусе (как обработчик горячих клавиш)
        // The capture phase at the board sees every key before the focused field (as a hot key handler does)
        struct KeyLog
        {
            gran::Application* application;

            void operator()(gran::ui::Node&, gran::ui::UiEvent& event) const
            {
                std::string text = "Последняя клавиша: ";

                if ((event.modifiers & gran::platform::MODIFIER_CONTROL) != 0)
                    text += "Ctrl+";
                if ((event.modifiers & gran::platform::MODIFIER_ALT) != 0)
                    text += "Alt+";
                if ((event.modifiers & gran::platform::MODIFIER_SHIFT) != 0)
                    text += "Shift+";
                text += gran::platform::keyName(event.key);
                if (event.target != nullptr && !event.target->id.empty())
                    text += " → " + event.target->id;
                setText(*application, "keyInfo", text);
            }
        };

        // focus и blur поля — в строку состояния (related — откуда или куда ушёл фокус) / The field focus and blur go into the status line (related is where the focus came from or went to)
        struct FieldFocus
        {
            gran::Application* application;

            void operator()(gran::ui::Node&, gran::ui::UiEvent& event) const
            {
                if (event.type == gran::ui::EventType::FOCUS)
                    setText(*application, "status", "Поле в фокусе");
                else
                    setText(*application, "status", "Поле потеряло фокус");
            }
        };
    }

    // Имена из samples/events.xml / The names from samples/events.xml
    void registerKeys(gran::Application& application)
    {
        application.onEvent("keyLog", KeyLog{&application});
        application.onEvent("fieldFocus", FieldFocus{&application});
        application.onEvent("fieldBlur", FieldFocus{&application});
    }
}
