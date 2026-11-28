#include "gran/app/internal/app_state.hpp"
#include "gran/core/i18n.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/log/log.hpp"
#include "gran/markup/live_document.hpp"
#include "gran/markup/overlay.hpp"
#include "gran/platform/host.hpp"

#include <algorithm>

namespace gran::app
{
    namespace
    {
        // Обновляем именованный набор и активное имя только после установки окна.
        // Update the named set and active name only after the window is installed.
        void storeProfile(Application::State& state, const Application::State::ProfileChange& profile)
        {
            auto names = state.settings.getList("gran.profiles");
            if (std::find(names.begin(), names.end(), profile.name) == names.end())
                names.push_back(profile.name);
            state.settings.setList("gran.profiles", names);
            state.settings.setString("gran.profile." + profile.name, profile.xml);
            state.settings.setString("gran.activeProfile", profile.name);
            refreshLayoutChoices(state);
        }

        // Копия допускает самоотписку; ошибка наблюдателя не отменяет уже установленный профиль.
        // The copy permits self-unsubscription; observer errors cannot roll back an installed profile.
        void notifyLayout(Application::State& state)
        {
            if (auto notify = state.layoutChanged)
            {
                try { notify(); }
                catch (const std::exception& error) { log::write(log::Level::WARNING, "layout", error.what()); }
                catch (...) { log::write(log::Level::WARNING, "layout", i18n::format(i18n::Message::APP_RELOAD_FAILED)); }
            }
        }
    }

    // Предпросмотр сохраняет базу для отката, обычная правка также обновляет настройки.
    // Preview retains the base for rollback; a committed edit also updates settings.
    void commitProfile(Application::State& state, Application::State::ProfileChange change)
    {
        const bool wasPreview = state.previewActive;
        state.previewActive = change.preview;
        if (change.preview)
        {
            state.currentProfile.base = std::move(change.base);
            notifyLayout(state);
            return;
        }
        state.currentProfile = std::move(change);
        const auto& profile = state.currentProfile;
        if (!profile.persist)
        {
            if (wasPreview) notifyLayout(state);
            return;
        }
        storeProfile(state, profile);
        if (!profile.undoXml.empty())
        {
            auto* app = state.owner;
            app->pushUndo([app, name = profile.name, xml = profile.undoXml] { app->importProfile(name, xml); },
                [app, name = profile.name, xml = profile.xml] { app->importProfile(name, xml); });
        }
        notifyLayout(state);
    }

    // После обновления разметки сохранённый профиль снова проходит полную проверку.
    // Revalidate the stored profile after the base markup changes.
    void stageBaseDocument(Application::State& state, markup::UiDocument document, const std::string& path)
    {
        Application::State::ProfileChange profile;
        if (!state.options.skipCustomization)
            profile.name = state.settings.getString("gran.activeProfile");
        if (!profile.name.empty() && state.windows.empty() && !state.options.headless && platform::bypassProfileKey())
            state.options.skipCustomization = true;
        if (state.options.skipCustomization) profile.name.clear();
        if (!profile.name.empty())
        {
            profile.xml = state.settings.getString("gran.profile." + profile.name);
            profile.base = std::make_shared<const std::string>(markup::serializeMarkup(document));
            try
            {
                auto result = markup::applyOverlay(document, profile.xml, {});
                for (const auto& id : result.missingIds)
                    log::write(log::Level::WARNING, "markup", i18n::format(i18n::Message::LINT_REMOVED_ID, {id}));
                document = std::move(result.document);
            }
            catch (const GranException& error)
            {
                log::write(log::Level::WARNING, "markup", error.what());
                profile = {};
            }
        }
        stageDocument(state, std::move(document), path, std::move(profile));
    }
}
