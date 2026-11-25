#include "gran/app/internal/app_state.hpp"
#include "gran/res/resources.hpp"

#include "gran/utils/file_system.hpp"

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
        app::startSettings(*_state);
    }

    // Работники завершают post до разрушения окон / Workers finish posting before window teardown
    Application::~Application()
    {
        _state->driverScope.reset();
        _state->pool.reset();
    }

    // Разметка из файла: разбор и проверка схемой за один проход / Markup from a file: parsing and the schema check in one pass
    void Application::loadFile(const std::string& path)
    {
        // Абсолютный путь: наблюдение и reload не зависят от смены текущего каталога; res:/ — уже абсолютный
        // An absolute path: watching and reload do not depend on changing the current directory; res:/ is already absolute
        std::string absolute = path;

        if (!res::isResourcePath(path))
            absolute = fs::absolutePath(path);

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
