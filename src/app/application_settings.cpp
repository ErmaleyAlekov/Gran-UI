#include "gran/app/internal/app_state.hpp"

namespace gran
{
    namespace app
    {
        /*
        ** Файл настроек: свой путь (settingsFile) или по имени программы (appName); без обоих настройки
        ** живут только в памяти и не сохраняются. Испорченный файл уже разобран load (в .bad).
        **
        ** The settings file: a custom path (settingsFile) or by the program name (appName); without both the settings
        ** live only in memory and are not saved. A damaged file is already handled by load (into .bad).
        */
        void startSettings(Application::State& state)
        {
            const AppOptions& options = state.options;

            if (!options.settingsFile.empty())
                state.settings.load(options.settingsFile);
            else if (!options.appName.empty())
                state.settings.load(settings::defaultPath(options.appName));
        }

        // Конец run(): состояние всех окон — в настройки, файл — на диск, если что-то изменилось / The end of run(): the state of all the windows goes into the settings, the file — to disk if something changed
        void saveSettings(Application::State& state)
        {
            for (const std::unique_ptr<AppWindow>& win : state.windows)
                saveState(*win);
            if (state.settings.modified())
                state.settings.save();
        }
    }

    // Настройки программы (значения, раскладка) / The program settings (the values, the layout)
    settings::Settings& Application::settings()
    {
        return _state->settings;
    }
}
