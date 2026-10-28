#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/layout/layout.hpp"

#include <chrono>

namespace gran
{
    // Узел по id / A node by id
    ui::Node* Application::find(const std::string& id)
    {
        if (!_state->document.root)
            return nullptr;
        return ui::findNode(*_state->document.root, id);
    }

    // Новый стиль — пересчёт каскада узла и его детей (с переходами, если они описаны) / A new style — recomputing the cascade of the node and its children (with transitions if described)
    void Application::setStyle(ui::Node& node, const std::string& styleName)
    {
        const style::StyleRule* rule = _state->document.styles.find(styleName);

        if (rule == nullptr)
            throw GranException(ErrorCode::RESOURCE, i18n::format(i18n::Message::RESOURCE_UNKNOWN_STYLE, {styleName}));
        if (node.rule == rule)
            return;
        node.rule = rule;
        ui::markDirty(node, ui::DIRTY_STYLE | ui::DIRTY_PAINT);
    }

    namespace app
    {
        namespace
        {
            // Фокус ввода → дерево доступности; смена — событие для дикторов / The input focus → the accessibility tree; a change is an event for screen readers
            void syncAccessFocus(Application::State& state)
            {
                const ui::Node* focus = state.router->focused();

                if (focus == state.access.focus)
                    return;
                state.access.focus = focus;
                state.window->accessFocusChanged();
            }
        }

        // Монотонные часы от первого вызова / The monotonic clock from the first call
        double now()
        {
            static const std::chrono::steady_clock::time_point START = std::chrono::steady_clock::now();
            return std::chrono::duration<double>(std::chrono::steady_clock::now() - START).count();
        }

        /*
        ** Кадр: стили (с наблюдателем-аниматором), шаг анимаций, компоновка только при
        ** DIRTY_LAYOUT у корня, отрисовка с кареткой фокусированного поля. Потеря устройства —
        ** пересоздание графики и повтор кадра со следующим проходом цикла.
        **
        ** A frame: styles (with the animator observer), an animation step, layout only with
        ** DIRTY_LAYOUT on the root, rendering with the caret of the focused input. A device loss —
        ** recreating the graphics and repeating the frame on the next loop pass.
        */
        void renderFrame(Application::State& state)
        {
            ui::Node& root = *state.document.root;
            const markup::Theme& theme = markup::defaultTheme();
            float scale = state.window->scale();
            Size pixels = state.window->pixelSize();
            const ui::Node* caret = state.router->focused();

            state.animator.setTime(now());
            ui::resolveStyles(root, state.windowStyle, theme.kinds, &state.animator);
            state.animating = state.animator.tick();
            if ((root.dirty & ui::DIRTY_LAYOUT) != 0)
                layoutFrame(state, Size{pixels.width / scale, pixels.height / scale});
            revealCaret(state);
            if (caret == nullptr || !ui::isTextField(*caret) || !state.caretVisible)
                caret = nullptr;
            if (state.renderer->render(root, state.windowStyle, scale, caret, state.router->selected())
                == rhi::FrameStatus::DEVICE_LOST)
                recreateGraphics(state);
            state.window->setCursor(state.router->cursor());
            syncAccessFocus(state);
            state.window->accessRefresh();
            syncWebViews(state);
        }
    }
}
