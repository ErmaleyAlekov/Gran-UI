#include "gran/platform/x11/x11_window.hpp"
#include <X11/Xatom.h>
#include <dlfcn.h>

/*
** Синхронизация смены размера с оконным менеджером (EWMH _NET_WM_SYNC_REQUEST). Перед каждой сменой размера мышью
** менеджер (Mutter в GNOME) шлёт окну номер запроса и не показывает окно нового размера, пока окно не запишет этот
** номер в свой счётчик XSync — то есть пока не нарисует кадр нового размера. Без этого Mutter показывал окно раньше
** кадра GPU, и у растущего правого края мерцала полоса фона (найдено в ручной проверке Т-20).
**
** Счётчик XSync живёт в libXext; она есть на любом рабочем столе X11, но её пакета разработчика может не быть,
** поэтому библиотека загружается при запуске (dlopen), как загрузчик Vulkan, и не добавляет зависимостей сборки.
** Нет библиотеки или расширения — окно работает, как раньше, без синхронизации.
**
** Resize synchronization with the window manager (EWMH _NET_WM_SYNC_REQUEST). Before every mouse resize the manager
** (Mutter in GNOME) sends the window a request number and does not show the window of the new size until the window
** writes that number into its XSync counter — that is, until it has drawn a frame of the new size. Without it Mutter
** showed the window before the GPU frame, and a background strip flickered at the growing right edge (found in the
** manual check T-20).
**
** The XSync counter lives in libXext; it exists on every X11 desktop, but its developer package may be missing,
** so the library is loaded at run time (dlopen), like the Vulkan loader, and adds no build dependencies.
** No library or extension — the window works as before, without synchronization.
*/
namespace gran::platform::x11
{
    namespace
    {
        // Раскладка XSyncValue из <X11/extensions/sync.h> / The XSyncValue layout from <X11/extensions/sync.h>
        struct SyncValue
        {
            int hi;
            unsigned int lo;
        };

        using SyncQuery = int (*)(Display*, int*, int*);
        using SyncCreate = XID (*)(Display*, SyncValue);
        using SyncSet = int (*)(Display*, XID, SyncValue);
        using SyncDestroy = int (*)(Display*, XID);

        // Функции XSync из libXext; все nullptr — синхронизации нет / The XSync functions from libXext; all nullptr — no synchronization
        struct SyncApi
        {
            SyncQuery query = nullptr;
            SyncQuery initialize = nullptr;
            SyncCreate create = nullptr;
            SyncSet set = nullptr;
            SyncDestroy destroy = nullptr;
        };

        // Библиотека не выгружается: её функции нужны до конца процесса
        // The library is never unloaded: its functions are needed until the process ends
        SyncApi loadSync()
        {
            SyncApi api;
            void* library = dlopen("libXext.so.6", RTLD_NOW | RTLD_LOCAL);

            if (library == nullptr)
                return api;
            api.query = reinterpret_cast<SyncQuery>(dlsym(library, "XSyncQueryExtension"));
            api.initialize = reinterpret_cast<SyncQuery>(dlsym(library, "XSyncInitialize"));
            api.create = reinterpret_cast<SyncCreate>(dlsym(library, "XSyncCreateCounter"));
            api.set = reinterpret_cast<SyncSet>(dlsym(library, "XSyncSetCounter"));
            api.destroy = reinterpret_cast<SyncDestroy>(dlsym(library, "XSyncDestroyCounter"));
            if (api.query == nullptr || api.initialize == nullptr || api.create == nullptr || api.set == nullptr
                || api.destroy == nullptr)
                return SyncApi{};
            return api;
        }

        // Загрузка один раз; инициализация статической переменной потокобезопасна (C++11)
        // Loaded once; initializing a static variable is thread-safe (C++11)
        const SyncApi& syncApi()
        {
            static const SyncApi api = loadSync();

            return api;
        }
    }

    /*
    ** Протоколы окна: закрытие крестиком и, если есть XSync, синхронизация смены размера. Счётчик создаётся
    ** на соединении окна (у каждого окна своё) и объявляется менеджеру свойством _NET_WM_SYNC_REQUEST_COUNTER.
    **
    ** The window protocols: closing by the cross and, if XSync exists, resize synchronization. The counter is created
    ** on the window connection (every window has its own) and announced to the manager by _NET_WM_SYNC_REQUEST_COUNTER.
    */
    void X11Window::protocols()
    {
        Atom names[2] = {atom("WM_DELETE_WINDOW"), atom("_NET_WM_SYNC_REQUEST")};
        const SyncApi& api = syncApi();
        int events = 0;
        int errors = 0;
        int major = 3;
        int minor = 1;

        if (api.create != nullptr && api.query(_native.display, &events, &errors) != 0
            && api.initialize(_native.display, &major, &minor) != 0)
            _syncCounter = api.create(_native.display, SyncValue{0, 0});
        if (_syncCounter == 0)
        {
            XSetWMProtocols(_native.display, _native.window, names, 1);
            return;
        }
        // Свойство формата 32 передаётся массивом long / A format-32 property is passed as an array of long
        unsigned long counter = _syncCounter;
        XChangeProperty(_native.display, _native.window, atom("_NET_WM_SYNC_REQUEST_COUNTER"), XA_CARDINAL, 32,
            PropModeReplace, reinterpret_cast<unsigned char*>(&counter), 1);
        XSetWMProtocols(_native.display, _native.window, names, 2);
    }

    /*
    ** Кадр показан. Если менеджер ждёт ответа и после его запроса уже пришёл ConfigureNotify (кадр нарисован
    ** под новый размер), номер запроса пишется в счётчик. XFlush — сразу: иначе ответ ждал бы следующего запроса
    ** к серверу, и Mutter держал бы окно замороженным.
    **
    ** A frame was shown. If the manager waits for an answer and a ConfigureNotify has already come after its request
    ** (the frame was drawn for the new size), the request number is written into the counter. XFlush — at once:
    ** otherwise the answer would wait for the next request to the server, and Mutter would keep the window frozen.
    */
    void X11Window::framePresented()
    {
        if (!_syncPending || !_syncConfigured)
            return;
        SyncValue value{static_cast<int>(_syncHigh), static_cast<unsigned int>(_syncLow)};
        syncApi().set(_native.display, _syncCounter, value);
        XFlush(_native.display);
        _syncPending = false;
        _syncConfigured = false;
    }

    // Счётчик уничтожается до закрытия соединения (Native) / The counter is destroyed before the connection closes (Native)
    void X11Window::destroySync()
    {
        if (_syncCounter != 0 && _native.display != nullptr)
            syncApi().destroy(_native.display, _syncCounter);
        _syncCounter = 0;
    }
}
