#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/layout/layout.hpp"

#include <chrono>

namespace gran
{
    // Узел по id: окна по порядку, главное первым / A node by id: the windows in order, the main one first
    ui::Node* Application::find(const std::string& id)
    {
        for (const std::unique_ptr<app::AppWindow>& win : _state->windows)
        {
            ui::Node* found = nullptr;

            if (win->document.root)
                found = ui::findNode(*win->document.root, id);
            if (found != nullptr)
                return found;
        }
        return nullptr;
    }

    // Новый стиль (из документа окна узла) — пересчёт каскада узла и его детей (с переходами, если они описаны)
    // A new style (from the node window document) — recomputing the cascade of the node and its children (with transitions if described)
    void Application::setStyle(ui::Node& node, const std::string& styleName)
    {
        const style::StyleRule* rule = app::windowOf(*_state, node).document.styles.find(styleName);

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
            // После кадра: ответ оконному менеджеру (X11), курсор, фокус ввода → дерево доступности (смена — событие
            // для дикторов)
            // After a frame: the answer to the window manager (X11), the cursor, the input focus → the accessibility
            // tree (a change is an event for screen readers)
            void syncAfterFrame(AppWindow& win, rhi::FrameStatus status)
            {
                // У списка строк таблицы и дерева дикторам — текущая строка или ячейка / For a table and tree row list the screen readers get the current row or cell
                const ui::Node* focus = accessFocusOf(win, win.router->focused());

                // Менеджер ждёт этот кадр: ответ — когда кадр уже в окне (Т-24)
                // The manager waits for this frame: the answer when the frame is already in the window (T-24)
                if (win.window->waitsForFrame() && !win.backend->canConfirmPresentation())
                {
                    win.backend->waitPresented();
                    win.window->framePresented();
                    win.presentationRetry = -1;
                }
                else if (win.window->waitsForFrame())
                {
                    const auto request = win.window->frameRequest();
                    if (win.presentationRetry < 0 || request != win.presentationRequest || win.presentationId == 0)
                    {
                        win.presentationRequest = request;
                        win.presentationId = status == rhi::FrameStatus::PRESENTED ? win.backend->lastPresentation() : 0;
                        win.presentationRetry = now() + 0.016;
                    }
                }
                else
                    win.presentationRetry = -1;
                win.window->setCursor(win.router->cursor());
                if (focus == win.access.focus)
                    return;
                win.access.focus = focus;
                win.window->accessFocusChanged();
            }
        }

        // Монотонные часы от первого вызова / The monotonic clock from the first call
        double now()
        {
            static const std::chrono::steady_clock::time_point START = std::chrono::steady_clock::now();
            return std::chrono::duration<double>(std::chrono::steady_clock::now() - START).count();
        }

        void pollFramePresentation(AppWindow& win)
        {
            win.presentationRetry = -1;
            if (!win.window->waitsForFrame() || win.presentationRequest != win.window->frameRequest())
                return;
            using P = rhi::RenderBackend::Presentation;
            switch (win.backend->pollPresentation(win.presentationId))
            {
                case P::SHOWN: win.window->framePresented(); break;
                case P::PENDING: win.presentationRetry = now() + 0.016; break;
                case P::DEVICE_LOST: win.deviceLost = true; win.changed = true; break;
                case P::RETRY_FRAME: win.changed = true; break;
            }
        }

        /*
        ** Кадр: состояние команд отмены, стили (с наблюдателем-аниматором), шаг анимаций, компоновка только при
        ** DIRTY_LAYOUT у корня, отрисовка с кареткой фокусированного поля. Потеря устройства —
        ** пересоздание графики и повтор кадра со следующим проходом цикла.
        **
        ** A frame: the undo commands state, styles (with the animator observer), an animation step, layout only with
        ** DIRTY_LAYOUT on the root, rendering with the caret of the focused input. A device loss —
        ** recreating the graphics and repeating the frame on the next loop pass.
        */
        void renderFrame(AppWindow& win, bool recoverDevice)
        {
            if (win.deviceLost)
            {
                if (!recoverDevice) return;
                recreateGraphics(win);
                win.deviceLost = false;
            }
            ui::Node& root = *win.document.root;
            const markup::Theme& theme = markup::defaultTheme();
            float scale = win.window->scale();
            Size pixels = win.window->pixelSize();
            const ui::Node* caret = win.router->focused();

            win.animator.setTime(now());
            syncWindowFrame(win);
            // Текст поля мог смениться из кода (история сбрасывается): включённость gran:undo — до стилей кадра
            // The field text might change from code (the history is reset): the gran:undo enabled state — before the frame styles
            refreshUndoCommands(win.shared, std::string());
            // Выражения {…}: ввод и код с прошлого кадра могли сменить их зависимости / The {…} expressions: the input and the code since the previous frame might change their dependencies
            win.shared.lists.updateExpressions();
            ui::resolveStyles(root, win.windowStyle, theme.kinds, &win.animator);
            win.animating = win.animator.tick();
            if ((root.dirty & ui::DIRTY_LAYOUT) != 0)
                layoutFrame(win, Size{pixels.width / scale, pixels.height / scale});
            revealCaret(win);
            if (caret == nullptr || !ui::isTextField(*caret) || !win.caretVisible)
                caret = nullptr;
            const auto status = win.renderer->render(root, win.windowStyle, scale,
                render::FrameMarks{caret, win.router->selected(), win.router->mnemonicsShown()});
            if (status == rhi::FrameStatus::DEVICE_LOST)
            {
                win.deviceLost = true;
                win.changed = true;
                if (!recoverDevice) return;
                recreateGraphics(win);
                win.deviceLost = false;
                return;
            }
            checkGraphicsDiagnostics(win);
            syncAfterFrame(win, status);
            win.window->accessRefresh();
            win.access.changed = nullptr;
            win.access.added.clear();
            announceStatus(win);
            syncWebViews(win);
            syncDropZones(win);
        }
    }
}
