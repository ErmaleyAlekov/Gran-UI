#include "gran/app/internal/app_state.hpp"

namespace gran
{
    namespace
    {
        /*
        ** Диалог ОС с владельцем — окном узла или главным (до run() окон ОС нет — без владельца). Без ОС
        ** (options.headless) ответ берётся из заготовленных тестом путей: настоящий диалог остановил бы тест.
        **
        ** The OS dialog with the owner — the node window or the main one (before run() there are no OS windows — no owner).
        ** Without an OS (options.headless) the answer comes from the paths prepared by a test: a real dialog would stop the test.
        */
        std::vector<std::string> askPaths(Application::State& state, platform::FileDialogKind kind,
            const FileDialogOptions& options, const ui::Node* owner)
        {
            app::AppWindow* win = nullptr;
            std::vector<std::string> answer;

            if (state.options.headless)
            {
                if (!state.dialogAnswers.empty())
                {
                    answer = std::move(state.dialogAnswers.front());
                    state.dialogAnswers.pop_front();
                }
                return answer;
            }
            if (owner != nullptr)
                win = app::findWindow(state, *owner);
            if (win == nullptr && !state.windows.empty())
                win = state.windows.front().get();
            if (win == nullptr)
                return platform::showFileDialog(kind, options, nullptr);
            return platform::showFileDialog(kind, options, win->window.get());
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
        return askPaths(*_state, kind, options, owner);
    }

    // Путь для сохранения / The path to save to
    std::string Application::saveFileDialog(const FileDialogOptions& options, const ui::Node* owner)
    {
        return firstPath(askPaths(*_state, platform::FileDialogKind::SAVE, options, owner));
    }

    // Путь папки / The folder path
    std::string Application::folderDialog(const FileDialogOptions& options, const ui::Node* owner)
    {
        return firstPath(askPaths(*_state, platform::FileDialogKind::FOLDER, options, owner));
    }
}
