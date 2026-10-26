#include "gran/app/internal/app_state.hpp"

#include <filesystem>
#include <utility>

namespace gran
{
    namespace
    {
        // После загрузки: начальные состояния, стиль окна, шрифты <Font> относительно каталога разметки
        // After loading: the initial states, the window style, the <Font> fonts relative to the markup directory
        void adoptDocument(Application::State& state, const std::string& base)
        {
            ui::applyInitialStates(*state.document.root);
            state.windowStyle = markup::windowStyle(markup::defaultTheme(), state.document.window);
            for (const markup::FontFace& face : state.document.fonts)
                state.fonts.addFile((std::filesystem::path(base) / face.source).string());
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
        _state->document = markup::loadMarkupFile(path, markup::LoadOptions{});
        adoptDocument(*_state, std::filesystem::path(path).parent_path().string());
    }

    // Разметка из строки (шрифты — относительно текущего каталога) / Markup from a string (the fonts are relative to the current directory)
    void Application::loadString(std::string_view markup)
    {
        _state->document = markup::loadMarkupString(markup, markup::LoadOptions{});
        adoptDocument(*_state, ".");
    }

    // Обработчик заменяет прежний с тем же именем / The handler replaces a former one with the same name
    void Application::onAction(const std::string& name, ActionHandler handler)
    {
        _state->actions[name] = std::move(handler);
    }
}
