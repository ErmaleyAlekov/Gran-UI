#include "gran/app/internal/app_state.hpp"

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

        // Отключаем потребителей дерева и очищаем очередь UI Automation до удаления узлов.
        // Disconnect tree consumers and clear the UI Automation queue before deleting nodes.
        void detachDocument(AppWindow& win)
        {
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
        }

        // Окно и бэкенд сохраняются; рендерер получает новый каталог ресурсов и кэши.
        // Keep the window and backend; the renderer receives the new resource directory and caches.
        void attachDocument(AppWindow& win)
        {
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
        }
    }

    // На границе цикла обработчики уже не используют старые узлы главного окна.
    // At a loop boundary handlers no longer use the old main window nodes.
    bool applyReload(Application::State& state)
    {
        if (!state.pendingDocument || state.windows.empty())
            return false;
        AppWindow& win = *state.windows.front();
        std::unique_ptr<markup::UiDocument> document = std::move(state.pendingDocument);
        std::string path = std::move(state.pendingPath);
        ReloadGuard guard(state);

        state.pendingPath.clear();
        detachDocument(win);
        adoptDocument(win, std::move(*document), sourceBase(path));
        rememberSource(state, path);
        attachDocument(win);
        return true;
    }
}
