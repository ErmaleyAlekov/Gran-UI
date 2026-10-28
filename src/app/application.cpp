#include "gran/app/internal/app_state.hpp"

#include <filesystem>
#include <utility>

namespace gran
{
    namespace
    {
        // Документ главного окна: первое окно создаётся, повторная загрузка заменяет его документ
        // The main window document: the first window is created, a repeated load replaces its document
        void adoptMain(Application::State& state, markup::UiDocument document, const std::string& base)
        {
            if (state.windows.empty())
                app::addWindow(state, std::move(document), base);
            else
                app::adoptDocument(*state.windows.front(), std::move(document), base);
        }
    }

    // Состояние создаётся сразу; окно — при run() / The state is created at once; the window — at run()
    Application::Application(AppOptions options)
        : _state(std::make_unique<State>())
    {
        _state->options = options;
    }

    // Порядок разрушения задан порядком полей State / The destruction order is set by the State field order
    Application::~Application() = default;

    // Разметка из файла: разбор и проверка схемой за один проход / Markup from a file: parsing and the schema check in one pass
    void Application::loadFile(const std::string& path)
    {
        adoptMain(*_state, markup::loadMarkupFile(path, markup::LoadOptions{}),
            std::filesystem::path(path).parent_path().string());
    }

    // Разметка из строки (шрифты — относительно текущего каталога) / Markup from a string (the fonts are relative to the current directory)
    void Application::loadString(std::string_view markup)
    {
        adoptMain(*_state, markup::loadMarkupString(markup, markup::LoadOptions{}), ".");
    }

    // Обработчик заменяет прежний с тем же именем / The handler replaces a former one with the same name
    void Application::onAction(const std::string& name, ActionHandler handler)
    {
        _state->actions[name] = std::move(handler);
    }
}
