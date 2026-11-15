#include "gran/platform/cocoa/internal/cocoa_window_impl.hpp"
#include "gran/core/i18n.hpp"

namespace
{
    /*
    ** Строка меню macOS: только меню программы с пунктом «Завершить» (Cmd+Q). Меню команд строит программа
    ** (в разметке это кнопки и всплывающие меню), а Cmd+C, Cmd+V и остальные сочетания приходят в окно как
    ** KEY_DOWN с Mod: без пунктов меню AppKit их не перехватывает.
    **
    ** The macOS menu bar: only the program menu with the "Quit" item (Cmd+Q). The command menus are built by the program
    ** (buttons and popup menus in the markup), and Cmd+C, Cmd+V and the other chords come to the window as KEY_DOWN
    ** with Mod: without menu items AppKit does not intercept them.
    */
    NSMenu* buildMenu()
    {
        NSMenu* bar = [[NSMenu alloc] init];
        NSMenuItem* program = [[NSMenuItem alloc] init];
        NSMenu* menu = [[NSMenu alloc] init];
        const char* process = [NSProcessInfo processInfo].processName.UTF8String;
        std::string name;

        // UTF8String бывает NULL (имя без представления UTF-8) / UTF8String may be NULL (a name without a UTF-8 form)
        if (process != nullptr)
            name = process;
        std::string quit = gran::i18n::format(gran::i18n::Message::PLATFORM_MENU_QUIT, {name});
        NSString* title = [NSString stringWithUTF8String:quit.c_str()];
        if (title == nil)
            title = @"Quit";
        [menu addItemWithTitle:title action:@selector(terminate:) keyEquivalent:@"q"];
        program.submenu = menu;
        [bar addItem:program];
        return bar;
    }
}

namespace gran::platform::cocoa
{
    /*
    ** Приложение AppKit один раз на процесс: обычная программа с Dock и строкой меню, даже если она запущена
    ** из терминала без пакета .app (иначе окно не получает фокус). Если программа сама уже настроила NSApp
    ** (встраивание в приложение Cocoa), её делегат и меню не трогаются.
    **
    ** The AppKit application once per process: a regular program with the Dock and the menu bar, even when it is run
    ** from a terminal without an .app bundle (otherwise the window gets no focus). If the program has already set up
    ** NSApp itself (embedding into a Cocoa application), its delegate and menu are not touched.
    */
    void ensureApplication()
    {
        // Сильная ссылка: NSApp.delegate — слабая / A strong reference: NSApp.delegate is weak
        static GranCocoaAppDelegate* delegate = nil;

        if (delegate != nil)
            return;
        // Ссылки на якоря затягивают файлы категорий в программу (cocoa_window_impl.hpp)
        // The anchor references pull the category files into the program (cocoa_window_impl.hpp)
        linkViewInput();
        linkTextInput();
        linkTextMarked();
        linkTextQuery();
        linkDelegateState();
        [NSApplication sharedApplication];
        delegate = [[GranCocoaAppDelegate alloc] init];
        if (NSApp.activationPolicy == NSApplicationActivationPolicyProhibited)
            [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        if (NSApp.delegate == nil)
            NSApp.delegate = delegate;
        if (NSApp.mainMenu == nil)
            NSApp.mainMenu = buildMenu();
        if (!NSApp.running)
            [NSApp finishLaunching];
    }
}

@implementation GranCocoaAppDelegate

// «Завершить» не завершает процесс сразу: каждое окно получает обычный CLOSE, решение — за программой
// "Quit" does not end the process at once: every window gets the usual CLOSE, the decision is the program's
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender
{
    (void)sender;
    for (gran::platform::cocoa::CocoaWindow* window : gran::platform::cocoa::CocoaWindow::registry())
        window->push(gran::platform::EventKind::CLOSE);
    return NSTerminateCancel;
}

@end
