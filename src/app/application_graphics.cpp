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

        // Бэкенд под текущий размер окна в пикселях / The backend for the current window size in pixels
        void createBackend(Application::State& state)
        {
            Size pixels = state.window->pixelSize();
            rhi::BackendOptions options;

            options.nativeWindow = state.window->nativeHandle();
            options.width = static_cast<std::uint32_t>(pixels.width);
            options.height = static_cast<std::uint32_t>(pixels.height);
            options.vsync = state.options.vsync;
            options.debugLayer = state.options.debugLayer;
            state.backend = rhi::createPlatformBackend(options);
            state.renderer = std::make_unique<render::Renderer>(*state.backend, state.fonts,
                state.document.baseDirectory);
        }
    }

    // Окно по <Window>, бэкенд, рендерер, маршрутизатор ввода / The window by <Window>, the backend, the renderer, the input router
    void createGraphics(Application::State& state)
    {
        const markup::WindowSpec& spec = state.document.window;
        platform::WindowDesc desc;

        desc.title = spec.title;
        desc.width = spec.width;
        desc.height = spec.height;
        desc.minWidth = spec.minWidth;
        desc.minHeight = spec.minHeight;
        desc.resizable = spec.resizable;
        desc.darkTitleBar = darkBackground(state.windowStyle);
        state.window = platform::createWindow(desc);
        state.access.root = state.document.root.get();
        state.window->setAccessTree(&state.access);
        createBackend(state);
        state.router = std::make_unique<input::InputRouter>(*state.document.root, state, state.window.get(), state);
        createTooltip(state);
        if (state.pendingFocus != nullptr)
            state.router->focus(state.pendingFocus);
    }

    // Старый рендерер отдаёт текстуры старому бэкенду до его разрушения / The old renderer returns the textures to the old backend before it is destroyed
    void recreateGraphics(Application::State& state)
    {
        state.renderer.reset();
        state.backend.reset();
        createBackend(state);
        ui::markDirty(*state.document.root, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
    }
}
