#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/log/log.hpp"
#include "gran/markup/live_document.hpp"
#include "gran/markup/overlay.hpp"
#include "gran/utils/utf8.hpp"

namespace gran
{
    namespace
    {
        // Имя сохраняется в XML настроек, поэтому проверяем управляющие символы и UTF-8.
        // The name is persisted in settings XML, so check control characters and UTF-8.
        bool validName(const std::string& name)
        {
            for (std::size_t offset = 0; offset < name.size();)
            {
                const auto start = offset;
                const auto ch = decodeUtf8(name, offset);
                if (ch < 32 || ch == 0xfffe || ch == 0xffff || (ch == REPLACEMENT_CHARACTER && offset - start == 1))
                    return false;
            }
            return !name.empty();
        }
    }

    // Проверяем размер до копирования; публикация профиля идёт через обычную замену окна.
    // Check the size before copying; publish the profile through the normal window replacement.
    void Application::importProfile(const std::string& name, std::string_view xml)
    {
        markup::checkOverlaySize(xml.size());
        if (!validName(name) || _state->windows.empty() || (_state->pendingDocument && !_state->pendingProfile.preview) || _state->reloading)
            throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::OVERLAY_DENIED, {name}));
        State::ProfileChange profile{_state->currentProfile.base, name, std::string(xml), true, {}};
        if (_state->recordLayoutEdit)
            profile.undoXml = _state->currentProfile.effectiveXml();
        app::stageProfileXml(*_state, std::move(profile), xml);
    }

    // Сохранённый профиль проходит ту же проверку, что и импортированный файл.
    // A saved profile undergoes the same validation as an imported file.
    void Application::selectProfile(const std::string& name)
    {
        importProfile(name, exportProfile(name));
    }

    // Возвращаем операции сохранённого профиля для переноса в другое приложение той же версии.
    // Return the saved operations for transfer to another installation.
    std::string Application::exportProfile(const std::string& name) const
    {
        if (!_state->settings.find("gran.profile." + name))
            throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::OVERLAY_DENIED, {name}));
        return _state->settings.getString("gran.profile." + name);
    }

    // Порядок имён соответствует сохранённому списку настроек приложения.
    // Names follow the persisted order in the application settings.
    std::vector<std::string> Application::profiles() const
    {
        return _state->settings.getList("gran.profiles");
    }
}
