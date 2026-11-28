#include "gran/app/internal/app_state.hpp"
#include "gran/binding/internal/reload_checkpoint.hpp"
#include "gran/ui/node_state.hpp"
#include "gran/log/log.hpp"

namespace gran::app
{
    namespace
    {
        // Рендерер и ввод готовятся для будущего дерева, получателем событий остаётся живое окно.
        // Prepare rendering and input for the incoming tree while keeping the live window as the event sink.
        void prepareWindow(AppWindow& win, AppWindow& next, const ui::TreeState& saved)
        {
            next.pendingFocus = ui::restoreState(*next.document.root, saved, false);
            if (win.shared.lists.refreshGridFilters(*next.document.root))
                next.pendingFocus = ui::restoreState(*next.document.root, saved, false);
            if (!win.window) return;
            next.renderer = std::make_unique<render::Renderer>(*win.backend, win.shared.fonts, next.document.baseDirectory);
            next.router = std::make_unique<input::InputRouter>(*next.document.root, win, win.window.get(), win);
            createTooltip(next);
        }

        // Отсоединяем потребителей старых узлов перед уничтожением документа.
        // Disconnect consumers of old nodes before destroying the document.
        void detach(AppWindow& win)
        {
            if (win.window) win.window->setAccessTree(nullptr);
            win.access.root = nullptr;
            win.access.focus = nullptr;
            win.access.changed = nullptr;
            win.access.added.clear();
            win.access.requests.clear();
            win.router.reset();
            win.renderer.reset();
            win.gridEdit.reset();
            win.dropNode = nullptr;
            win.revealNode = nullptr;
            win.tooltipSource = nullptr;
            win.tooltipShown = win.tooltipSuppressed = false;
            forgetWebViews(win, *win.document.root);
            ++win.shared.treeVersion;
        }

        void install(AppWindow& win, AppWindow& next)
        {
            detach(win);
            win.document = std::move(next.document);
            win.persistedVariables = std::move(next.persistedVariables);
            win.windowStyle = std::move(next.windowStyle);
            win.animator = std::move(next.animator);
            win.animator.setStyleSheet(&win.document.styles);
            win.renderer = std::move(next.renderer);
            win.router = std::move(next.router);
            win.tooltip = next.tooltip;
            win.tooltipText = next.tooltipText;
            win.pendingFocus = next.pendingFocus;
            if (!win.window) return;
            win.access.root = win.document.root.get();
            win.window->setAccessTree(&win.access);
            win.animating = win.caretVisible = win.changed = true;
            ui::markDirty(*win.document.root, ui::DIRTY_STYLE | ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
        }

        // Сначала восстанавливаем источники, затем поля и фокус старого дерева.
        // Restore the sources first, then the fields and focus of the old tree.
        void restoreWindow(AppWindow& win, AppWindow& next, const ui::TreeState& saved,
            std::optional<text::FontLibrary>& fonts, binding::ReloadCheckpoint& checkpoint)
        {
            if (fonts) win.shared.fonts = std::move(*fonts);
            if (next.document.root) win.shared.lists.discardDocumentBindings(*next.document.root);
            checkpoint.restore();
            win.shared.lists.updateExpressions();
            auto* focus = ui::restoreState(*win.document.root, saved, false);
            if (!win.router) win.pendingFocus = focus;
            if (focus && win.router)
            {
                try { win.router->focus(focus); }
                catch (...) { data::internal::ObserverErrors::capture(); }
            }
        }
    }

    bool replaceDocument(AppWindow& win, markup::UiDocument document, const std::string& base)
    {
        AppWindow next(win.shared);
        next.replacing = &win;
        auto saved = ui::captureState(*win.document.root, win.router ? win.router->focused() : win.pendingFocus);
        binding::ReloadCheckpoint checkpoint(win.shared.lists, document);
        std::optional<text::FontLibrary> fonts;
        if (!document.fonts.empty()) fonts.emplace(win.shared.fonts);
        try
        {
            win.shared.lists.releaseDocumentFilters(*win.document.root);
            checkpoint.check();
            adoptDocument(next, std::move(document), base);
            checkpoint.check();
            prepareWindow(win, next, saved);
            checkpoint.check();
            if (win.shared.applyingProfile)
                moveApplicationChildren(win, next);
            if ((win.shared.applyingProfile || win.shared.previewActive) && next.router && win.router)
                next.router->adoptProfileInput(*win.router, layoutInspector(next), next.pendingFocus);
        }
        catch (...)
        {
            restoreWindow(win, next, saved, fonts, checkpoint);
            return reportReloadFailure(win.shared);
        }
        win.shared.lists.discardDocumentBindings(*win.document.root);
        install(win, next);
        return true;
    }
}
