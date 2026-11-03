#include "gran/app/internal/app_state.hpp"

#include <filesystem>
#include <utility>

namespace gran
{
    // Состояние создаётся сразу; окно — при run() / The state is created at once; the window — at run()
    Application::Application(AppOptions options)
        : _state(std::make_unique<State>())
    {
        _state->owner = this;
        _state->options = options;
        app::startLogging(*_state);
    }

    // Порядок разрушения задан порядком полей State / The destruction order is set by the State field order
    Application::~Application() = default;

    // Разметка из файла: разбор и проверка схемой за один проход / Markup from a file: parsing and the schema check in one pass
    void Application::loadFile(const std::string& path)
    {
        // Абсолютный путь: наблюдение и reload не зависят от смены текущего каталога
        // An absolute path: watching and reload do not depend on changing the current directory
        std::string absolute = std::filesystem::absolute(path).lexically_normal().string();

        app::stageDocument(*_state, markup::loadMarkupFile(absolute, markup::LoadOptions{}), absolute);
    }

    // Разметка из строки (шрифты — относительно текущего каталога) / Markup from a string (the fonts are relative to the current directory)
    void Application::loadString(std::string_view markup)
    {
        app::stageDocument(*_state, markup::loadMarkupString(markup, markup::LoadOptions{}), "");
    }

    // Обработчик заменяет прежний с тем же именем / The handler replaces a former one with the same name
    void Application::onAction(const std::string& name, ActionHandler handler)
    {
        _state->actions[name] = std::move(handler);
    }
}
