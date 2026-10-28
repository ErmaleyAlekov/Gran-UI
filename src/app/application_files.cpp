#include "gran/app/internal/app_state.hpp"

namespace gran
{
    namespace
    {
        // Окно ОС для владельца диалога: окно узла или главное; до run() окон ОС нет — без владельца
        // The OS window for the dialog owner: the node window or the main one; before run() there are no OS windows — no owner
        platform::Window* ownerWindow(Application::State& state, const ui::Node* owner)
        {
            app::AppWindow* win = nullptr;

            if (owner != nullptr)
                win = app::findWindow(state, *owner);
            if (win == nullptr && !state.windows.empty())
                win = state.windows.front().get();
            if (win == nullptr)
                return nullptr;
            return win->window.get();
        }

        // Первый путь или пусто / The first path or empty
        std::string firstPath(const std::vector<std::string>& paths)
        {
            if (paths.empty())
                return std::string();
            return paths.front();
        }
    }

    // Один или несколько файлов / One or several files
    std::vector<std::string> Application::openFileDialog(const FileDialogOptions& options, bool multiple,
        const ui::Node* owner)
    {
        platform::FileDialogKind kind = platform::FileDialogKind::OPEN;

        if (multiple)
            kind = platform::FileDialogKind::OPEN_MULTIPLE;
        return platform::showFileDialog(kind, options, ownerWindow(*_state, owner));
    }

    // Путь для сохранения / The path to save to
    std::string Application::saveFileDialog(const FileDialogOptions& options, const ui::Node* owner)
    {
        return firstPath(platform::showFileDialog(platform::FileDialogKind::SAVE, options, ownerWindow(*_state, owner)));
    }

    // Путь папки / The folder path
    std::string Application::folderDialog(const FileDialogOptions& options, const ui::Node* owner)
    {
        return firstPath(platform::showFileDialog(platform::FileDialogKind::FOLDER, options,
            ownerWindow(*_state, owner)));
    }
}
