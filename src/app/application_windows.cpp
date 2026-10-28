#include "gran/app/internal/app_state.hpp"

#include <filesystem>
#include <utility>

namespace gran
{
    // Окно из файла: шрифты и картинки — относительно каталога файла / A window from a file: the fonts and images are relative to the file directory
    ui::Node& Application::openWindow(const std::string& path)
    {
        app::AppWindow& win = app::addWindow(*_state, markup::loadMarkupFile(path, markup::LoadOptions{}),
            std::filesystem::path(path).parent_path().string());

        return *win.document.root;
    }

    // Окно из строки: относительные пути — от текущего каталога / A window from a string: relative paths are from the current directory
    ui::Node& Application::openWindowString(std::string_view markup)
    {
        app::AppWindow& win = app::addWindow(*_state, markup::loadMarkupString(markup, markup::LoadOptions{}), ".");

        return *win.document.root;
    }

    // Закрытие откладывается до конца прохода: обработчик, вызвавший его, ещё работает с узлами окна
    // Closing is deferred to the pass end: the handler that called it still works with the window nodes
    void Application::closeWindow(ui::Node& node)
    {
        app::AppWindow* win = app::findWindow(*_state, node);

        if (win != nullptr)
            win->closing = true;
    }

    namespace app
    {
        // Корень дерева узла — корень документа одного из окон / The node tree root is the document root of one of the windows
        AppWindow* findWindow(Application::State& state, const ui::Node& node) noexcept
        {
            const ui::Node* root = &node;

            while (root->parent != nullptr)
                root = root->parent;
            for (const std::unique_ptr<AppWindow>& win : state.windows)
            {
                if (win->document.root.get() == root)
                    return win.get();
            }
            return nullptr;
        }

        // Маршрутизатор есть только у показанного окна / Only a shown window has a router
        input::InputRouter* routerOf(Application::State& state, const ui::Node& node) noexcept
        {
            AppWindow* win = findWindow(state, node);

            if (win == nullptr)
                return nullptr;
            return win->router.get();
        }
    }
}
