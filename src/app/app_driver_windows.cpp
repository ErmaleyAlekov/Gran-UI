#include "gran/app/app_driver.hpp"
#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <string>

namespace gran
{
    namespace
    {
        // Окно ОС приложения — безоконное (AppDriver требует headless); нет окна — ошибка аргумента
        // The application OS window is windowless (AppDriver requires headless); no window — an argument error
        platform::HeadlessWindow& headlessOf(app::AppWindow* win, const std::string& what)
        {
            if (win == nullptr || !win->window)
                throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::APP_DRIVER_NO_WINDOW, {what}));
            return static_cast<platform::HeadlessWindow&>(*win->window);
        }
    }

    std::size_t AppDriver::windowCount() const
    {
        return state().windows.size();
    }

    platform::HeadlessWindow& AppDriver::window(std::size_t index)
    {
        Application::State& current = state();
        app::AppWindow* win = nullptr;

        if (index < current.windows.size())
            win = current.windows[index].get();
        return headlessOf(win, std::to_string(index));
    }

    platform::HeadlessWindow& AppDriver::windowOf(const ui::Node& node)
    {
        return headlessOf(app::findWindow(state(), node), node.id);
    }

    void AppDriver::answerFileDialog(const std::vector<std::string>& paths)
    {
        state().dialogAnswers.push_back(paths);
    }
}
