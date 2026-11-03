#include "gran/app/internal/app_state.hpp"

#include <filesystem>
#include <utility>

namespace gran
{
    // Окно из файла: шрифты и картинки — относительно каталога файла / A window from a file: the fonts and images are relative to the file directory
    // Владелец проверяется до загрузки: узел чужого дерева — ошибка без нового окна
    // The owner is checked before loading: a node of a foreign tree is an error without a new window
    ui::Node& Application::openWindow(const std::string& path, const WindowOptions& options)
    {
        app::AppWindow* owner = app::requireOwner(*_state, options);
        app::AppWindow& win = app::addWindow(*_state, markup::loadMarkupFile(path, markup::LoadOptions{}),
            std::filesystem::path(path).parent_path().string());

        app::attachOwner(win, owner, options);
        return *win.document.root;
    }

    // Окно из строки: относительные пути — от текущего каталога / A window from a string: relative paths are from the current directory
    ui::Node& Application::openWindowString(std::string_view markup, const WindowOptions& options)
    {
        app::AppWindow* owner = app::requireOwner(*_state, options);
        app::AppWindow& win = app::addWindow(*_state, markup::loadMarkupString(markup, markup::LoadOptions{}), ".");

        app::attachOwner(win, owner, options);
        return *win.document.root;
    }

    // Закрытие откладывается до конца прохода: обработчик, вызвавший его, ещё работает с узлами окна
    // Closing is deferred to the pass end: the handler that called it still works with the window nodes
    void Application::closeWindow(ui::Node& node)
    {
        closeWindow(node, std::string());
    }

    // Результат запоминается до закрытия; повторный вызов в том же проходе заменяет его
    // The result is remembered until closing; a repeated call in the same pass replaces it
    void Application::closeWindow(ui::Node& node, const std::string& result)
    {
        app::markClosing(*_state, node, result);
    }
}
