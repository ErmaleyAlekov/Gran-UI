#include "gran/app/internal/app_state.hpp"
#include "gran/ui/node_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/log/log.hpp"
#include <utility>

namespace gran::app
{
    namespace
    {
        // На время замены цикл не рисует промежуточное состояние окна.
        // The loop skips rendering the intermediate window state during replacement.
        struct ReloadGuard
        {
            explicit ReloadGuard(Application::State& owner) : state(owner) { state.reloading = true; }
            ~ReloadGuard() { state.reloading = false; state.applyingProfile = false; }
            ReloadGuard(const ReloadGuard&) = delete;
            ReloadGuard& operator=(const ReloadGuard&) = delete;

            Application::State& state;
        };

    }

    // Вызывается из обработчика исключения. Вне цикла исходное исключение получает вызывающий код.
    // Called from an exception handler. Outside the loop the caller receives the original exception.
    bool reportReloadFailure(Application::State& state)
    {
        if (!state.running) throw;
        try { throw; }
        catch (const std::exception& error) { state.reloadFailure = error.what(); }
        catch (...) { state.reloadFailure = i18n::format(i18n::Message::APP_RELOAD_FAILED); }
        log::write(log::Level::SEVERE, "markup", state.reloadFailure);
        return false;
    }

    // Выбираем общий снимок окна, если частичное восстановление потеряет состояние.
    // Choose a window-wide snapshot when a partial restore would lose state.
    static bool installReload(AppWindow& win, markup::UiDocument& document, bool profile, const std::string& path, bool& full)
    {
        auto& state = win.shared;
        try
        {
            state.lists.validateDocument(document);
        }
        catch (const GranException&)
        {
            return reportReloadFailure(state);
        }
        state.applyingProfile = profile;
        full = (state.applyingProfile && hasApplicationChildren(*win.document.root)) || !patchDocument(win, document);
        if (full)
        {
            if (!replaceDocument(win, std::move(document), sourceBase(path))) return false;
        }
        return true;
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
        auto stamps = std::move(state.pendingLibraryStamps);
        auto profile = std::move(state.pendingProfile);
        ReloadGuard guard(state);

        state.pendingPath.clear();
        bool full = false;
        if (!installReload(win, *document, profile.base || state.currentProfile.base, path, full)) return false;
        rememberSource(state, path);
        state.libraryStamps = std::move(stamps);
        commitProfile(state, std::move(profile));
        if (full && win.router && win.pendingFocus)
        {
            auto* focus = std::exchange(win.pendingFocus, nullptr);
            try { win.router->focus(focus); }
            catch (const std::exception& error) { log::write(log::Level::WARNING, "focus", error.what()); }
            catch (...) { log::write(log::Level::WARNING, "focus", "onFocus"); }
        }
        return true;
    }
}
