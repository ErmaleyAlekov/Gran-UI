#include "gran/app/internal/app_state.hpp"
#include "gran/ui/node_state.hpp"

namespace gran::app
{
    namespace
    {
        /*
        ** Флаг замены снимается и при исключении (сторож RAII): иначе окно навсегда перестало бы
        ** рисоваться. Ошибка посередине замены (GPU, привязки) выходит из run() — прежнее дерево
        ** к этому моменту уже удалено, продолжать с полусобранным окном нельзя.
        **
        ** The replacement flag is cleared on an exception too (an RAII guard): otherwise the window would never
        ** render again. An error in the middle of the replacement (GPU, bindings) leaves run() — the former tree
        ** is already deleted by then, continuing with a half-built window is impossible.
        */
        struct ReloadGuard
        {
            explicit ReloadGuard(Application::State& owner) : state(owner) { state.reloading = true; }
            ~ReloadGuard() { state.reloading = false; }
            ReloadGuard(const ReloadGuard&) = delete;
            ReloadGuard& operator=(const ReloadGuard&) = delete;

            Application::State& state;
        };

        // Снимок состояния пользователя, затем отключаем потребителей дерева и очищаем очередь UI Automation до удаления узлов.
        // A snapshot of the user state, then disconnect tree consumers and clear the UI Automation queue before deleting nodes.
        ui::TreeState detachDocument(AppWindow& win)
        {
            ui::TreeState saved;

            if (win.document.root && win.router)
                saved = ui::captureState(*win.document.root, win.router->focused());
            else if (win.document.root)
                saved = ui::captureState(*win.document.root, nullptr);
            if (win.window)
                win.window->setAccessTree(nullptr);
            win.access.root = nullptr;
            win.access.focus = nullptr;
            win.access.requests.clear();
            win.router.reset();
            win.renderer.reset();
            win.pendingFocus = nullptr;
            win.dropNode = nullptr;
            win.revealNode = nullptr;
            win.tooltip = nullptr;
            win.tooltipText = nullptr;
            win.tooltipSource = nullptr;
            win.tooltipShown = false;
            win.tooltipSuppressed = false;
            return saved;
        }

        /*
        ** Состояние пользователя переходит на новые узлы по id или месту. Окно и бэкенд сохраняются;
        ** рендерер получает новый каталог ресурсов и кэши; фокус — узлу, совпавшему с прежним.
        **
        ** The user state moves to the new nodes by id or place. Keep the window and backend;
        ** the renderer receives the new resource directory and caches; the focus goes to the node matching the former one.
        */
        void attachDocument(AppWindow& win, const ui::TreeState& saved)
        {
            ui::Node* focus = nullptr;

            if (win.document.root)
                focus = ui::restoreState(*win.document.root, saved, false);
            if (!win.window)
                return;
            win.renderer = std::make_unique<render::Renderer>(*win.backend, win.shared.fonts, win.document.baseDirectory);
            win.router = std::make_unique<input::InputRouter>(*win.document.root, win, win.window.get(), win);
            win.access.root = win.document.root.get();
            win.window->setAccessTree(&win.access);
            createTooltip(win);
            win.animating = true;
            win.caretVisible = true;
            win.changed = true;
            ui::markDirty(*win.document.root, ui::DIRTY_STYLE | ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
            if (focus != nullptr)
                win.router->focus(focus);
        }
    }

    /*
    ** На границе цикла обработчики уже не используют старые узлы главного окна. Сначала — частичная
    ** замена (те же окно и ресурсы); не вышло — полная, с переносом состояния пользователя.
    **
    ** At a loop boundary handlers no longer use the old main window nodes. First — a partial
    ** replacement (the same window and resources); if it fails — a full one, carrying over the user state.
    */
    bool applyReload(Application::State& state)
    {
        if (!state.pendingDocument || state.windows.empty())
            return false;
        AppWindow& win = *state.windows.front();
        std::unique_ptr<markup::UiDocument> document = std::move(state.pendingDocument);
        std::string path = std::move(state.pendingPath);
        ReloadGuard guard(state);

        state.pendingPath.clear();
        if (!patchDocument(win, *document))
        {
            ui::TreeState saved = detachDocument(win);
            adoptDocument(win, std::move(*document), sourceBase(path));
            attachDocument(win, saved);
        }
        rememberSource(state, path);
        return true;
    }
}
