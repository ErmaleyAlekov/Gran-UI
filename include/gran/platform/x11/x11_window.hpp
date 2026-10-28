#ifndef GRAN_PLATFORM_X11_WINDOW_HPP
#define GRAN_PLATFORM_X11_WINDOW_HPP
#include "gran/platform/window.hpp"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#undef CursorShape
#include <deque>
#include <vector>

namespace gran::platform::x11
{
    // Владелец native-ресурсов освобождается и при исключении из конструктора окна.
    // The native resource owner is also destroyed if window construction throws.
    struct Native
    {
        ~Native();
        Display* display = nullptr;
        ::Window window = 0;
        XIM method = nullptr;
        XIC input = nullptr;
        int wakeRead = -1;
        int wakeWrite = -1;
        ::Cursor cursor = 0;
    };

    // X11 является системной оконной службой; Wayland-сеансы поддерживаются через XWayland.
    // X11 is the system window service; Wayland sessions are supported through XWayland.
    class X11Window final : public platform::Window
    {
    public:
        explicit X11Window(const WindowDesc& desc);
        ~X11Window() override;
        bool pollEvent(Event& event) override;
        void waitEvents(double timeout) override;
        void* nativeHandle() const override;
        void* nativeDisplay() const override;
        float scale() const override;
        Size pixelSize() const override;
        void setCursor(CursorShape shape) override;
        void show() override;
        void minimize() override;
        void maximize() override;
        void restore() override;
        void close() override;
        bool isMaximized() const override;
        void setFrameHitTest(std::function<WindowRole(Vec2)> callback) override;
        void setLiveResize(std::function<void()> callback) override;
        std::string clipboardText() override;
        void setClipboardText(const std::string& text) override;
        void setAccessTree(a11y::AccessTree* tree) override;
        void accessFocusChanged() override;
        void accessRefresh() override;
        void wake() noexcept override;
    private:
        void create(const WindowDesc& desc);
        void configure(const WindowDesc& desc);
        void inputMethod();
        void dispatch(XEvent& event);
        void mouse(XEvent& event);
        void button(const XButtonEvent& native, Event& event);
        void keyboard(XKeyEvent& event, bool down);
        void text(XKeyEvent& event);
        void selection(XEvent& event);
        void selectionRequest(const XSelectionRequestEvent& request);
        void sendState(long state);
        bool frameMouse(const XButtonEvent& event);
        void moveResize(const XButtonEvent& event, long direction);
        Atom atom(const char* name) const;
        void push(EventKind kind);
        static Key key(KeySym symbol);
        static Key physicalKey(unsigned keycode);
        static std::uint8_t modifiers(unsigned state);
        // Живые окна потока UI: у каждого своё соединение с X-сервером, а цикл приложения ждёт только
        // на главном окне — поэтому waitEvents слушает соединения всех окон.
        // The live windows of the UI thread: each has its own X server connection, while the application loop waits
        // only on the main window — so waitEvents listens to the connections of all the windows.
        static std::vector<X11Window*>& registry();
        bool pending();
        Native _native;
        std::deque<Event> _events;
        std::function<void()> _liveResize;
        std::function<WindowRole(Vec2)> _frameHit;
        std::string _clipboard;
        float _scale = 1;
        Size _pixels;
        bool _custom = false;
        bool _resizable = true;
        bool _clipboardReady = false;
        Time _lastClick = 0;
        unsigned _lastButton = 0;
        int _lastX = 0;
        int _lastY = 0;
    };
}
#endif
