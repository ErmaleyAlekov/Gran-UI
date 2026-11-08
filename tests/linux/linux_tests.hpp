#ifndef GRAN_LINUX_TESTS_HPP
#define GRAN_LINUX_TESTS_HPP
#include "gran/rhi/backend_factory.hpp"
#include "gran/platform/window.hpp"
#include <stdexcept>

namespace linux_tests
{
    struct Scene;
    void require(bool condition, const char* message);
    // Ключ --hardware: настоящая видеокарта (выбор — GRAN_ADAPTER) вместо lavapipe.
    // The --hardware key: a real video card (chosen by GRAN_ADAPTER) instead of lavapipe.
    bool& hardwareMode();
    void backend();
    void window();
    void input(gran::platform::Window& window);
    void clipboard(gran::platform::Window& window);
    int clipboardPeer();
    void validate(gran::rhi::RenderBackend& backend, gran::rhi::FrameData frame);
    void textures(gran::rhi::RenderBackend& backend, Scene& scene);
    void compare(gran::rhi::RenderBackend& backend, unsigned x, unsigned y, std::uint32_t color);
    // Данные теста живут дольше всех кадров; backend разрушается последним из GPU-потребителей.
    // Test data outlive every frame; backend is destroyed after all GPU consumers.
    struct Scene
    {
        explicit Scene(gran::rhi::RenderBackend& backend);
        gran::render::GpuClip clip{};
        gran::render::GpuInstance item{};
        gran::rhi::FrameSegment segment{};
        gran::rhi::FrameData frame;
        unsigned atlas = 0;
        unsigned ramp = 0;
    };
}
#endif
