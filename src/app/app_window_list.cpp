#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <utility>

namespace gran::app
{
    // Окно знает общее состояние; всё остальное — при загрузке документа и создании графики
    // The window knows the shared state; everything else comes on loading the document and creating the graphics
    AppWindow::AppWindow(Application::State& owner) noexcept
        : shared(owner)
    {
    }

    // Узел вне всех окон (копия шаблона до вставки) относится к главному окну; окон нет — разметка не загружена
    // A node outside all the windows (a template copy before insertion) belongs to the main window; no windows — the markup is not loaded
    AppWindow& windowOf(Application::State& state, const ui::Node& node)
    {
        AppWindow* win = findWindow(state, node);

        if (state.windows.empty())
            throw GranException(ErrorCode::MARKUP, i18n::format(i18n::Message::APP_NO_MARKUP));
        if (win == nullptr)
            return *state.windows.front();
        return *win;
    }

    // Окно с документом; графика — в openWindows (из цикла run) / A window with a document; the graphics — in openWindows (from the run loop)
    AppWindow& addWindow(Application::State& state, markup::UiDocument document, const std::string& base)
    {
        state.windows.push_back(std::make_unique<AppWindow>(state));
        adoptDocument(*state.windows.back(), std::move(document), base);
        return *state.windows.back();
    }

    /*
    ** Окна без графики получают окно ОС, бэкенд и первый кадр, затем показываются (без белой
    ** вспышки). Обход по индексу: обработчик в первом кадре может открыть ещё одно окно.
    **
    ** The windows without graphics get an OS window, a backend and the first frame, then are shown (without a white
    ** flash). A walk by index: a handler in the first frame may open one more window.
    */
    void openWindows(Application::State& state)
    {
        for (std::size_t index = 0; index < state.windows.size(); ++index)
        {
            AppWindow& win = *state.windows[index];

            if (win.window || win.closing)
                continue;
            createGraphics(win);
            renderFrame(win);
            win.window->show();
        }
    }

    /*
    ** Закрытие главного окна — выход из run() (остальные закроются вместе с приложением).
    ** Другое окно: снимаются привязки списков и сообщения его браузеров, затем окно уничтожается
    ** (порядок полей AppWindow). Окно, ещё не показанное, закрывается так же.
    **
    ** Closing the main window means leaving run() (the others close together with the application).
    ** Another window: its list bindings and its browser messages are dropped, then the window is destroyed
    ** (the AppWindow field order). A window not shown yet closes the same way.
    */
    void closeWindows(Application::State& state)
    {
        for (std::size_t index = 0; index < state.windows.size();)
        {
            AppWindow& win = *state.windows[index];

            if (!win.closing)
            {
                ++index;
                continue;
            }
            if (index == 0)
            {
                state.quit = true;
                return;
            }
            if (win.document.root)
            {
                state.lists.forget(*win.document.root);
                forgetWebViews(win, *win.document.root);
            }
            state.windows.erase(state.windows.begin() + static_cast<std::ptrdiff_t>(index));
        }
    }
}
