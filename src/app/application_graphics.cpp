#include "gran/app/internal/app_state.hpp"
#include "gran/rhi/backend_factory.hpp"

namespace gran::app
{
    namespace
    {
        // Тёмный ли фон окна (для тёмного заголовка Windows) / Whether the window background is dark (for the Windows dark title bar)
        bool darkBackground(const style::ComputedStyle& window) noexcept
        {
            const Color& color = window.background.color;

            if (window.background.kind != style::PaintKind::COLOR)
                return false;
            return 0.2126f * color.r + 0.7152f * color.g + 0.0722f * color.b < 0.5f;
        }

        /*
        ** Перерисовка окна во время изменения размера мышью (функтор вместо лямбды). Размер бэкенда
        ** и компоновка обновляются до синхронного кадра; во время замены документа (COM может
        ** доставить WM_SIZE внутри неё) частичное состояние не рисуется.
        **
        ** Repainting the window while resizing with the mouse (a functor instead of a lambda). The backend size
        ** and the layout are updated before the synchronous frame; during a document replacement (COM can
        ** deliver WM_SIZE inside it) a partial state is not rendered.
        */
        struct LiveResize
        {
            AppWindow* win;

            void operator()() const
            {
                if (win->shared.reloading || !win->backend || !win->renderer)
                    return;
                Size pixels = win->window->pixelSize();

                win->backend->resize(static_cast<std::uint32_t>(pixels.width), static_cast<std::uint32_t>(pixels.height));
                ui::markDirty(*win->document.root, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
                renderFrame(*win);
            }
        };

        // Бэкенд под текущий размер окна в пикселях / The backend for the current window size in pixels
        void createBackend(AppWindow& win)
        {
            Size pixels = win.window->pixelSize();
            rhi::BackendOptions options;

            options.nativeWindow = win.window->nativeHandle();
            options.nativeDisplay = win.window->nativeDisplay();
            options.width = static_cast<std::uint32_t>(pixels.width);
            options.height = static_cast<std::uint32_t>(pixels.height);
            options.vsync = win.shared.options.vsync;
            options.debugLayer = win.shared.options.debugLayer;
            options.backend = win.shared.options.backend;
            options.forceWarp = win.shared.options.forceWarp;
            win.backend = rhi::createPlatformBackend(options);
            win.renderer = std::make_unique<render::Renderer>(*win.backend, win.shared.fonts,
                win.document.baseDirectory);
        }
    }

    // Окно ОС по <Window> (с перерисовкой при изменении размера), бэкенд, рендерер, маршрутизатор ввода
    // The OS window by <Window> (with repainting while resizing), the backend, the renderer, the input router
    void createGraphics(AppWindow& win)
    {
        const markup::WindowSpec& spec = win.document.window;
        platform::WindowDesc desc;

        desc.title = spec.title;
        desc.width = spec.width;
        desc.height = spec.height;
        desc.minWidth = spec.minWidth;
        desc.minHeight = spec.minHeight;
        desc.resizable = spec.resizable;
        desc.titleBar = spec.titleBar;
        desc.darkTitleBar = darkBackground(win.windowStyle);
        win.window = platform::createWindow(desc);
        win.window->setLiveResize(LiveResize{&win});
        win.access.root = win.document.root.get();
        win.window->setAccessTree(&win.access);
        createBackend(win);
        win.router = std::make_unique<input::InputRouter>(*win.document.root, win, win.window.get(), win);
        connectWindowFrame(win);
        createTooltip(win);
        if (win.pendingFocus != nullptr)
            win.router->focus(win.pendingFocus);
    }

    // Старый рендерер отдаёт текстуры старому бэкенду до его разрушения / The old renderer returns the textures to the old backend before it is destroyed
    void recreateGraphics(AppWindow& win)
    {
        win.renderer.reset();
        win.backend.reset();
        createBackend(win);
        ui::markDirty(*win.document.root, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
    }
}
