#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran::app
{
    /*
    ** Окно владельца по узлу из WindowOptions. Узел вне окон (копия шаблона, узел закрытого окна)
    ** не может владеть окном: подчинённое держало бы висячую ссылку.
    **
    ** The owner window by the node from WindowOptions. A node outside the windows (a template copy, a node of a closed window)
    ** cannot own a window: the owned one would hold a dangling reference.
    */
    AppWindow* requireOwner(Application::State& state, const WindowOptions& options)
    {
        AppWindow* owner = nullptr;

        if (options.owner == nullptr)
            return nullptr;
        owner = findWindow(state, *options.owner);
        if (owner == nullptr)
            throw GranException(ErrorCode::MARKUP, i18n::format(i18n::Message::APP_OWNER_NOT_WINDOW));
        return owner;
    }

    // Пометить окно узла к закрытию с результатом (после прохода событий); узел не из окна — ничего
    // Mark the node window for closing with a result (after the event pass); a node not from a window — nothing
    void markClosing(Application::State& state, const ui::Node& node, const std::string& result)
    {
        AppWindow* win = findWindow(state, node);

        if (win == nullptr)
            return;
        win->closing = true;
        win->closeResult = result;
    }

    // Окно стоит в списке после владельца — значит, закрывается раньше него / The window stands in the list after the owner — so it closes before it
    void attachOwner(AppWindow& win, AppWindow* owner, const WindowOptions& options)
    {
        win.owner = owner;
        win.modal = options.modal && owner != nullptr;
        win.onClosed = options.onClosed;
    }

    /*
    ** Вид, место и владелец окна ОС. Окну с владельцем место DEFAULT означает центр владельца;
    ** вид dialog у модального задаёт и разметка, и сам флаг modal (подсказка X11).
    **
    ** The kind, the place and the owner of the OS window. For a window with an owner the DEFAULT place means the owner center;
    ** the dialog kind of a modal window is set both by the markup and by the modal flag itself (an X11 hint).
    */
    void ownWindow(const AppWindow& win, platform::WindowDesc& desc)
    {
        desc.kind = win.document.window.kind;
        desc.position = win.document.window.position;
        desc.modal = win.modal;
        if (win.owner == nullptr || !win.owner->window)
            return;
        desc.owner = win.owner->window.get();
        if (desc.position == platform::WindowPosition::DEFAULT)
            desc.position = platform::WindowPosition::CENTER_OWNER;
    }
}
