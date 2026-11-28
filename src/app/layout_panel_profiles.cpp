#include "gran/app/internal/layout_panel.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/utils/file_system.hpp"
#include "gran/markup/overlay.hpp"

#include <fstream>

namespace gran::app::panel
{
    // Имена профилей берутся из настроек программы.
    // Populate profile names from the application's settings.
    void profilePage(Element& tabs, Application::State& state)
    {
        auto& tab = page(tabs, Message::LAYOUT_PROFILE);
        auto& saved = field(tab, "saved", Message::LAYOUT_SWITCH, true);
        for (const auto& name : state.owner->profiles())
            option(saved, name, name);
        button(tab, "switch", Message::LAYOUT_SWITCH);
        field(tab, "profile", Message::LAYOUT_NEW_PROFILE, false);
        button(tab, "save", Message::LAYOUT_SAVE);
        button(tab, "import", Message::LAYOUT_IMPORT);
        button(tab, "export", Message::LAYOUT_EXPORT);
        button(tab, "reset", Message::LAYOUT_RESET);
    }

    namespace
    {
        // Файл выбирает пользователь через системный диалог сохранения.
        // The user selects the file through the system save dialog.
        void exportFile(Application& app, ui::Node& root, const std::string& xml)
        {
            const std::string path = app.saveFileDialog({}, &root);
            if (path.empty())
                return;
            std::ofstream out;
            fs::openFile(out, path);
            out << xml;
            out.close();
            if (!out)
                throw GranException(ErrorCode::IO, i18n::format(Message::IO_WRITE_FAILED, {path}));
        }

        // Проверяем размер до выделения буфера и передаём XML штатному загрузчику профилей.
        // Check the size before allocating a buffer and pass the XML to the regular profile loader.
        void importFile(Application& app, ui::Node& root, const std::string& name)
        {
            const auto paths = app.openFileDialog({}, false, &root);
            if (paths.empty())
                return;
            std::ifstream in;
            fs::openFile(in, paths.front(), std::ios::binary | std::ios::ate);
            const auto size = in.tellg();
            if (size < 0)
                throw GranException(ErrorCode::IO, i18n::format(Message::IO_READ_FAILED, {paths.front()}));
            markup::checkOverlaySize(static_cast<std::size_t>(size));
            std::string xml(static_cast<std::size_t>(size), '\0');
            in.seekg(0);
            if (!in.read(xml.data(), size) || in.peek() != std::ifstream::traits_type::eof())
                throw GranException(ErrorCode::IO, i18n::format(Message::IO_READ_FAILED, {paths.front()}));
            app.importProfile(name, xml);
        }
    }

    // Сброс области использует выбранный в главном окне идентификатор.
    // A regional reset uses the identifier selected in the main window.
    bool profileAction(AppWindow& win, const std::string& action)
    {
        auto& state = win.shared;
        auto& app = *state.owner;
        auto& root = *win.document.root;
        const auto& profile = state.currentProfile;
        const auto name = value(root, "profile");
        const std::string& xml = profile.effectiveXml();
        if (action == "switch") app.selectProfile(value(root, "saved"));
        else if (action == "save") app.importProfile(name, xml);
        else if (action == "export") exportFile(app, root, xml);
        else if (action == "import") importFile(app, root, name);
        else if (action == "reset") app.resetProfile();
        else return false;
        return true;
    }
}
