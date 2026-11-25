#include "gran/settings/internal/settings_xml.hpp"
#include "gran/core/i18n.hpp"
#include "gran/log/log.hpp"

#include "gran/utils/file_system.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>

namespace gran::settings
{
    namespace
    {
        // Весь файл в строку; false — файла нет или он не читается / The whole file into a string; false — no file or it is unreadable
        bool readText(const std::string& path, std::string& text)
        {
            std::ifstream input;
            std::ostringstream content;

            fs::openFile(input, path);
            if (!input)
                return false;
            content << input.rdbuf();
            text = content.str();
            return true;
        }

        // Испорченный файл остаётся рядом (.bad) для разбора, место освобождается для нового / A damaged file stays beside (.bad) for analysis, the place is freed for a new one
        void keepDamaged(const std::string& path)
        {
            fs::removePath(path + ".bad");
            fs::renamePath(path, path + ".bad");
        }
    }

    /*
    ** Windows — %APPDATA%\<программа> (переезжает с профилем), macOS — ~/Library/Application Support/<программа>,
    ** Linux — $XDG_CONFIG_HOME или ~/.config.
    **
    ** Windows — %APPDATA%\<program> (roams with the profile), macOS — ~/Library/Application Support/<program>,
    ** Linux — $XDG_CONFIG_HOME or ~/.config.
    */
    std::string defaultPath(const std::string& appName)
    {
        std::string base = fs::temporaryDirectory();
#if defined(_WIN32)
        const wchar_t* roaming = _wgetenv(L"APPDATA");
        if (roaming != nullptr && roaming[0] != L'\0')
            base = fs::fromWidePath(roaming, wcslen(roaming));
#elif defined(__APPLE__)
        const char* home = std::getenv("HOME");
        if (home != nullptr && home[0] != '\0')
            base = fs::joinPath(home, "Library/Application Support");
#else
        const char* config = std::getenv("XDG_CONFIG_HOME");
        const char* home = std::getenv("HOME");
        if (config != nullptr && config[0] == '/')
            base = config;
        else if (home != nullptr && home[0] != '\0')
            base = fs::joinPath(home, ".config");
#endif
        return fs::joinPath(fs::joinPath(base, appName), "settings.xml");
    }

    /*
    ** Нет файла — пустые настройки (первый запуск). Испорченный файл — в .bad, предупреждение в журнал,
    ** пустые настройки: программа запускается со значениями по умолчанию, а не падает.
    **
    ** No file — empty settings (the first run). A damaged file goes into .bad, a warning to the log,
    ** empty settings: the program starts with the default values instead of crashing.
    */
    bool Settings::load(const std::string& path)
    {
        std::string text;
        std::string detail;
        Model model;

        _path = path;
        _model = Model{};
        _modified = false;
        if (!readText(path, text))
            return true;
        if (internal::readSettings(text, model, detail))
        {
            _model = std::move(model);
            return true;
        }
        keepDamaged(path);
        std::string warning = i18n::format(i18n::Message::SETTINGS_DAMAGED, {path});
        if (!detail.empty())
            warning += " (" + detail + ")";
        log::write(log::Level::WARNING, "settings", warning);
        return false;
    }

    // Временный файл и переименование: сбой посреди записи не портит прежние настройки / A temporary file and renaming: a failure in the middle of writing does not damage the former settings
    bool Settings::save()
    {
        std::string temporary = _path + ".tmp";
        std::ofstream output;

        if (_path.empty())
            return false;
        const auto serialized = internal::writeSettings(_model);
        if (serialized.empty())
            return false;
        fs::createDirectories(fs::parentPath(_path));
        fs::openFile(output, temporary);
        output << serialized;
        // close до проверки: ошибка сброса буфера при закрытии иначе не видна, и недописанный .tmp заменил бы рабочие
        // настройки; недописанный .tmp удаляется (ревью Т-57)
        // close before the check: otherwise a buffer flush error on closing is invisible, and an incomplete .tmp would replace
        // the working settings; an incomplete .tmp is deleted (the T-57 review)
        output.close();
        if (!output || !fs::renamePath(temporary, _path))
        {
            fs::removePath(temporary);
            return false;
        }
        _modified = false;
        return true;
    }
}
