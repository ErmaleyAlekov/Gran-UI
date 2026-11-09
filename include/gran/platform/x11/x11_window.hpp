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

    /*
    ** Состояние приёма перетаскивания файлов по протоколу XDND (x11_drop*.cpp). Один сеанс за раз: новый
    ** XdndEnter сбрасывает прежний; сообщения от другого источника, чем в XdndEnter, не принимаются.
    **
    ** The state of receiving dragged files by the XDND protocol (x11_drop*.cpp). One session at a time: a new
    ** XdndEnter resets the former one; messages from a source other than in XdndEnter are not accepted.
    */
    struct DndState
    {
        // Окно-источник текущего перетаскивания; 0 — перетаскивания нет
        // The source window of the current drag; 0 — no drag
        ::Window source = 0;
        // Версия протокола источника (старший байт data.l[1] в XdndEnter)
        // The source protocol version (the high byte of data.l[1] in XdndEnter)
        int version = 0;
        // Источник предлагает список файлов text/uri-list
        // The source offers a text/uri-list file list
        bool uriList = false;
        // Последний ответ XdndStatus принял бросок (точка над зоной)
        // The last XdndStatus answer accepted the drop (the point over a zone)
        bool accepted = false;
        // После XdndDrop ждём SelectionNotify с данными
        // After XdndDrop we wait for SelectionNotify with the data
        bool converting = false;
        // Последняя точка в логических пикселях окна
        // The last point in window logical pixels
        Vec2 point;
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
        void activate() override;
        bool placement(WindowPlacement& placement) const override;
        void setPlacement(const WindowPlacement& placement) override;
        void framePresented() override;
        bool waitsForFrame() const override;
        void setDropZones(const std::vector<Rect>& zones) override;
        std::vector<std::string> takeDroppedFiles() override;
    private:
        // ---- x11_drop*.cpp: приём файлов по XDND / receiving files by XDND ----
        void dndMessage(const XClientMessageEvent& message);
        void dndEnter(const XClientMessageEvent& message);
        bool dndOffersUris(const XClientMessageEvent& message) const;
        void dndPosition(const XClientMessageEvent& message);
        bool dndInZone(Vec2 point) const;
        void dndDrop(const XClientMessageEvent& message);
        void dndReply(const char* type, long accepted, const char* action);
        void dndData(const XSelectionEvent& event);
        void dndEnd(EventKind kind);
        std::vector<std::string> dndReadPaths(Atom property);
        void applyOwner(const WindowDesc& desc);
        void place(const WindowDesc& desc);
        void create(const WindowDesc& desc);
        void configure(const WindowDesc& desc);
        unsigned long backgroundPixel(const Color& color) const;
        void inputMethod();
        void dispatch(XEvent& event);
        void resized(XEvent& event);
        void clientMessage(const XClientMessageEvent& message);
        void protocols();
        void destroySync();
        void mouse(XEvent& event);
        void button(const XButtonEvent& native, Event& event);
        void keyboard(XKeyEvent& event, bool down);
        void text(XKeyEvent& event);
        void selection(XEvent& event);
        void selectionRequest(const XSelectionRequestEvent& request);
        void sendState(long state);
        bool frameMouse(const XButtonEvent& event);
        void moveResize(const XButtonEvent& event, long direction);
        long frameEdge(int x, int y) const;
        void trackFrameCursor(const XMotionEvent& event);
        void applyCursor();
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
        // Развернуть после показа (место из настроек) / Maximize after showing (the place from the settings)
        bool _startMaximized = false;
        bool _clipboardReady = false;
        // Курсор интерфейса, край своей рамки под указателем (направление _NET_WM_MOVERESIZE, -1 — нет)
        // и форма из cursorfont, уже заданная окну: курсор пересоздаётся только при смене формы.
        // The interface cursor, the custom frame edge under the pointer (a _NET_WM_MOVERESIZE direction, -1 — none)
        // and the cursorfont shape already set on the window: the cursor is recreated only when the shape changes.
        CursorShape _shape = CursorShape::ARROW;
        long _edge = -1;
        unsigned _fontShape = 0;
        Time _lastClick = 0;
        unsigned _lastButton = 0;
        int _lastX = 0;
        int _lastY = 0;
        // Синхронизация смены размера (x11_sync.cpp): счётчик XSync, номер запроса менеджера, ждёт ли он ответа
        // и пришёл ли после запроса новый размер
        // Resize synchronization (x11_sync.cpp): the XSync counter, the manager request number, whether it waits
        // for an answer and whether the new size has come after the request
        XID _syncCounter = 0;
        unsigned long _syncLow = 0;
        long _syncHigh = 0;
        bool _syncPending = false;
        bool _syncConfigured = false;
        // Приём файлов (XDND): зоны в логических пикселях, очередь путей (по одному списку на событие DROP),
        // состояние текущего перетаскивания и выставлен ли уже XdndAware
        // Receiving files (XDND): the zones in logical pixels, the path queue (one list per DROP event),
        // the current drag state and whether XdndAware is already set
        std::vector<Rect> _dropZones;
        std::deque<std::vector<std::string>> _dropped;
        DndState _dnd;
        bool _dndAware = false;
    };
}
#endif
