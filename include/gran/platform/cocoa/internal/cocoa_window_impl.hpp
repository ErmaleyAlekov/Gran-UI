#ifndef GRAN_PLATFORM_COCOA_INTERNAL_WINDOW_IMPL_HPP
#define GRAN_PLATFORM_COCOA_INTERNAL_WINDOW_IMPL_HPP

/*
** Внутренности платформы Cocoa (macOS, шаг M6): только для файлов .mm из src/platform/cocoa (Objective-C++ с ARC).
**
** Устройство. Окно — свой подкласс NSWindow (GranCocoaNativeWindow): его sendEvent: и performKeyEquivalent: видят
** все события мыши и клавиатуры окна раньше видов, поэтому ввод переводится в события движка в одном месте
** (cocoa_mouse.mm, cocoa_view_input.mm), а не в десятке методов NSView и не через цепочку ответчиков AppKit.
** Содержимое — вид GranCocoaView, слой которого — CAMetalLayer (поверхность бэкенда Metal, nativeHandle). Текст
** приходит через interpretKeyEvents вида → NSTextInputClient (insertText:) — так набирают мёртвые клавиши, Option и
** методы ввода.
** Делегат окна (GranCocoaWindowDelegate) сообщает о закрытии, размере, фокусе и масштабе.
**
** Цикл событий. Программа не вызывает [NSApp run]: события берёт сам движок (nextEventMatchingMask:untilDate:)
** и раздаёт их [NSApp sendEvent:]; объекты Objective-C отвечают только тем, что кладут события в очередь своего
** CocoaWindow (паттерн Bridge, как у Win32 и X11). wake() из любого потока кладёт в очередь AppKit событие
** NSEventTypeApplicationDefined — postEvent:atStart: AppKit разрешает вызывать из других потоков.
**
** Владение. Объекты Objective-C держат сильные ссылки ARC в полях CocoaWindow; у видов, окна и делегата —
** невладеющий указатель owner на CocoaWindow. Деструктор CocoaWindow обнуляет owner у всех трёх раньше, чем
** отпускает их: событие, которое AppKit доставит позже, просто отбрасывается. Методы, вызываемые из
** Objective-C, — noexcept: исключение C++ не должно пройти через кадры AppKit (gpu_rules.md, раздел 3).
**
** Cocoa platform internals (macOS, step M6): for the .mm files of src/platform/cocoa only (Objective-C++ with ARC).
**
** Structure. The window is an own NSWindow subclass (GranCocoaNativeWindow): its sendEvent: and performKeyEquivalent:
** see all the window mouse and keyboard events before the views, so the input is turned into engine events in one
** place (cocoa_mouse.mm, cocoa_view_input.mm), not in a dozen NSView methods and not through the AppKit responder chain.
** The content is the GranCocoaView view whose layer is a CAMetalLayer (the Metal backend surface, nativeHandle). The
** text comes through the view interpretKeyEvents → NSTextInputClient (insertText:) — that is how dead keys, Option and
** input methods type.
** The window delegate (GranCocoaWindowDelegate) reports closing, the size, the focus and the scale.
**
** The event loop. The program does not call [NSApp run]: the engine takes the events itself
** (nextEventMatchingMask:untilDate:) and hands them to [NSApp sendEvent:]; the Objective-C objects only put events
** into the queue of their CocoaWindow (the Bridge pattern, as Win32 and X11). wake() from any thread puts an
** NSEventTypeApplicationDefined event into the AppKit queue — AppKit allows calling postEvent:atStart: from other threads.
**
** Ownership. The Objective-C objects are held by ARC strong references in the CocoaWindow fields; the views, the window
** and the delegate have a non-owning owner pointer to CocoaWindow. The CocoaWindow destructor clears owner in all three
** before releasing them: an event AppKit delivers later is simply dropped. The methods called from Objective-C are
** noexcept: a C++ exception must not pass through AppKit frames (gpu_rules.md, section 3).
*/
#if !defined(__OBJC__) || !__has_feature(objc_arc)
#error "cocoa_window_impl.hpp: only for Objective-C++ with ARC (-fobjc-arc)"
#endif

#include "gran/platform/window.hpp"
#include "gran/platform/cocoa/internal/cocoa_keys.hpp"

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

#include <deque>
#include <functional>
#include <string>
#include <vector>

namespace gran::platform::cocoa
{
    class CocoaWindow;

    // Подтип события пробуждения (NSEventTypeApplicationDefined) — «GR» / The wake event subtype (NSEventTypeApplicationDefined) — "GR"
    constexpr short WAKE_SUBTYPE = 0x4752;

    // Приложение AppKit: создаётся один раз, до первого окна (cocoa_app.mm) / The AppKit application: created once, before the first window (cocoa_app.mm)
    void ensureApplication();

    // Модификаторы NSEvent → биты MODIFIER_* (Cmd → CONTROL, Option → ALT, Control → META)
    // The NSEvent modifiers → the MODIFIER_* bits (Cmd → CONTROL, Option → ALT, Control → META)
    std::uint8_t modifiersFromFlags(NSEventModifierFlags flags) noexcept;

    /*
    ** Якоря компоновки категорий. Компоновщик берёт объектный файл из статической библиотеки только по ссылке на его
    ** символ, а у категории Objective-C символов нет: без ссылки файл категории не попал бы в программу, и AppKit
    ** получил бы «unrecognized selector» (так упал первый прогон M6 на раннере: -validAttributesForMarkedText). Каждый
    ** файл категории определяет пустой якорь, ensureApplication вызывает все — программе не нужен ключ -ObjC.
    **
    ** The category link anchors. The linker takes an object file from a static library only by a reference to its
    ** symbol, and an Objective-C category has no symbols: without a reference the category file would not get into the
    ** program, and AppKit would get "unrecognized selector" (the first M6 run on the runner crashed so:
    ** -validAttributesForMarkedText). Each category file defines an empty anchor, ensureApplication calls them all —
    ** the program needs no -ObjC key.
    */
    void linkViewInput() noexcept;
    void linkTextInput() noexcept;
    void linkTextMarked() noexcept;
    void linkTextQuery() noexcept;
    void linkDelegateState() noexcept;
}

// Вид содержимого: слой CAMetalLayer, клавиатура и ввод текста / The content view: the CAMetalLayer layer, the keyboard and the text input
@interface GranCocoaView : NSView <NSTextInputClient>
@property(nonatomic, assign) gran::platform::cocoa::CocoaWindow* owner;
// Набираемая методом ввода строка (IME) и выделение в ней / The string being composed by an input method (IME) and its selection
@property(nonatomic, strong) NSString* marked;
@property(nonatomic, assign) NSRange markedSelection;
- (instancetype)initWithOwner:(gran::platform::cocoa::CocoaWindow*)owner frame:(NSRect)frame;
@end

// Команды метода ввода и первый щелчок (cocoa_view_input.mm) / The input method commands and the first click (cocoa_view_input.mm)
@interface GranCocoaView (Input)
- (void)doCommandBySelector:(SEL)selector;
- (BOOL)acceptsFirstMouse:(NSEvent*)event;
@end

// Ввод текста NSTextInputClient: изменение (cocoa_text_input.mm) / The NSTextInputClient text input: changes (cocoa_text_input.mm)
@interface GranCocoaView (TextInput)
- (void)insertText:(id)text replacementRange:(NSRange)range;
- (void)setMarkedText:(id)text selectedRange:(NSRange)selected replacementRange:(NSRange)range;
- (void)unmarkText;
@end

// Ввод текста NSTextInputClient: набираемая строка (cocoa_text_marked.mm) / The NSTextInputClient text input: the composed string (cocoa_text_marked.mm)
@interface GranCocoaView (TextMarked)
- (BOOL)hasMarkedText;
- (NSRange)markedRange;
- (NSRange)selectedRange;
@end

// Ввод текста NSTextInputClient: вопросы метода ввода (cocoa_text_query.mm) / The NSTextInputClient text input: the input method questions (cocoa_text_query.mm)
@interface GranCocoaView (TextQuery)
- (NSArray<NSAttributedStringKey>*)validAttributesForMarkedText;
- (NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range actualRange:(NSRangePointer)actual;
- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actual;
- (NSUInteger)characterIndexForPoint:(NSPoint)point;
@end

// Окно: перехват клавиш и мыши до видов (cocoa_native_window.mm) / The window: intercepting the keys and the mouse before the views (cocoa_native_window.mm)
@interface GranCocoaNativeWindow : NSWindow
@property(nonatomic, assign) gran::platform::cocoa::CocoaWindow* owner;
@end

// Делегат окна: закрытие, размер, фокус, масштаб (cocoa_delegate*.mm) / The window delegate: closing, the size, the focus, the scale (cocoa_delegate*.mm)
@interface GranCocoaWindowDelegate : NSObject <NSWindowDelegate>
@property(nonatomic, assign) gran::platform::cocoa::CocoaWindow* owner;
@end

// Делегат приложения: «Завершить» (Cmd+Q, Dock) превращается в CLOSE всех окон (cocoa_app.mm)
// The application delegate: "Quit" (Cmd+Q, the Dock) turns into CLOSE of all the windows (cocoa_app.mm)
@interface GranCocoaAppDelegate : NSObject <NSApplicationDelegate>
@end

namespace gran::platform::cocoa
{
    /*
    ** Окно Cocoa (паттерн Bridge, как Win32Window и X11Window). Все методы — только в главном потоке (AppKit),
    ** кроме wake(). Координаты событий — в пунктах вида (логических пикселях, начало — левый верхний угол:
    ** вид перевёрнут, isFlipped), размеры RESIZE и pixelSize — в пикселях устройства (backing).
    **
    ** A Cocoa window (the Bridge pattern, as Win32Window and X11Window). All the methods are on the main thread only
    ** (AppKit), except wake(). The event coordinates are in view points (logical pixels, the origin is the top left
    ** corner: the view is flipped, isFlipped), the RESIZE sizes and pixelSize are in device (backing) pixels.
    */
    class CocoaWindow final : public platform::Window
    {
    public:
        explicit CocoaWindow(const WindowDesc& desc);
        ~CocoaWindow() override;
        CocoaWindow(const CocoaWindow&) = delete;
        CocoaWindow& operator=(const CocoaWindow&) = delete;

        bool pollEvent(Event& event) override;
        void waitEvents(double timeout) override;
        void* nativeHandle() const override;
        float scale() const override;
        Size pixelSize() const override;
        void setCursor(CursorShape shape) override;
        void show() override;
        void minimize() override;
        void maximize() override;
        void restore() override;
        void close() override;
        bool isMaximized() const override;
        void activate() override;
        bool placement(WindowPlacement& placement) const override;
        void setPlacement(const WindowPlacement& placement) override;
        void setFrameHitTest(std::function<WindowRole(Vec2)> callback) override;
        void setLiveResize(std::function<void()> callback) override;
        std::string clipboardText() override;
        void setClipboardText(const std::string& text) override;
        void setAccessTree(a11y::AccessTree* tree) override;
        void accessFocusChanged() override;
        void accessRefresh() override;
        void wake() noexcept override;

        // ---- Вызовы из Objective-C (noexcept) / Calls from Objective-C (noexcept) ----

        // Событие мыши окна; true — поглощено (перетаскивание своей рамки), AppKit его не получает (cocoa_mouse.mm)
        // A window mouse event; true — consumed (dragging the custom frame), AppKit does not get it (cocoa_mouse.mm)
        bool mouse(NSEvent* event) noexcept;
        // Событие клавиатуры окна; true — это клавиша, AppKit её дальше не передаёт (cocoa_view_input.mm)
        // A window keyboard event; true — it is a key, AppKit does not pass it further (cocoa_view_input.mm)
        bool key(NSEvent* event) noexcept;
        // Клавиша нажата / отпущена; keyDown возвращает, передавать ли клавишу методу ввода (cocoa_keyboard.mm)
        // A key is pressed / released; keyDown returns whether to pass the key to the input method (cocoa_keyboard.mm)
        bool keyDown(NSEvent* event) noexcept;
        void keyUp(NSEvent* event) noexcept;
        // Зафиксированный текст метода ввода → CHARACTER по символу UTF-32 / The committed input method text → CHARACTER per UTF-32 symbol
        void insertText(NSString* text) noexcept;
        // Размер вида или масштаб сменились (RESIZE / DPI_CHANGED); во время перетаскивания края — перерисовка
        // The view size or the scale changed (RESIZE / DPI_CHANGED); while the edge is dragged — a repaint
        void resized() noexcept;
        void backingChanged() noexcept;
        // Короткое событие без данных (CLOSE, FOCUS_*, EXPOSE, MOUSE_LEAVE) / A short event without data (CLOSE, FOCUS_*, EXPOSE, MOUSE_LEAVE)
        void push(EventKind kind) noexcept;
        // Курсор для прямоугольника курсора вида (resetCursorRects) / The cursor for the view cursor rectangle (resetCursorRects)
        NSCursor* cursor() const noexcept;

        // Живые окна потока интерфейса: «Завершить» закрывает все, waitEvents не спит, если очередь любого не пуста
        // The live windows of the UI thread: "Quit" closes all, waitEvents does not sleep if any queue is not empty
        static std::vector<CocoaWindow*>& registry();

    private:
        // Создание (cocoa_create.mm) / Creation (cocoa_create.mm)
        void create(const WindowDesc& desc);
        void configure(const WindowDesc& desc);
        void place(const WindowDesc& desc);
        // Событие с данными в очередь; при нехватке памяти событие теряется, а не бросает в AppKit
        // An event with data into the queue; on running out of memory the event is lost instead of throwing into AppKit
        void post(const Event& event) noexcept;
        // Кнопка и колесо (cocoa_mouse.mm) / The button and the wheel (cocoa_mouse.mm)
        Vec2 point(NSEvent* event) const noexcept;
        void button(NSEvent* event, Event& result) noexcept;
        bool frameMouse(NSEvent* event, Vec2 position) noexcept;
        void wheel(NSEvent* event, Vec2 position) noexcept;
        // Раздать события AppKit: первое ждём до until, остальные — без ожидания (cocoa_events.mm)
        // Hand out the AppKit events: the first one is awaited until until, the rest — without waiting (cocoa_events.mm)
        static void pump(NSDate* until);

        // Порядок полей: делегат и вид освобождаются раньше окна (обратный порядок объявления), но связь с ними
        // разрывает деструктор явно (owner = nil, delegate = nil) — порядок здесь не задаёт безопасность.
        // The field order: the delegate and the view are released before the window (the reverse declaration order),
        // but the destructor breaks the links explicitly (owner = nil, delegate = nil) — the order does not provide safety here.
        GranCocoaNativeWindow* _window = nil;
        GranCocoaView* _view = nil;
        CAMetalLayer* _layer = nil;
        GranCocoaWindowDelegate* _delegate = nil;
        // Окно-владелец (слабая ссылка): подчинённое окно становится его дочерним при показе, а не при создании —
        // addChildWindow: сразу выводит окно на экран, до первого кадра
        // The owner window (a weak reference): the owned window becomes its child on showing, not on creation —
        // addChildWindow: puts the window on screen at once, before the first frame
        NSWindow* __weak _parent = nil;
        std::deque<Event> _events;
        std::function<void()> _liveResize;
        std::function<WindowRole(Vec2)> _frameHit;
        a11y::AccessTree* _access = nullptr;
        CursorShape _shape = CursorShape::ARROW;
        // Последний сообщённый размер в пикселях устройства: одинаковый RESIZE не повторяется
        // The last reported size in device pixels: the same RESIZE is not repeated
        Size _pixels;
        bool _custom = false;
        bool _startMaximized = false;
        // Control+щелчок — правая кнопка (соглашение macOS); отпускание той же кнопки тоже правое
        // Control+click is the right button (the macOS convention); the release of that button is right too
        bool _controlClick = false;
    };
}

#endif
