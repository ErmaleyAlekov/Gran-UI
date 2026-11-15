#ifndef GRAN_RHI_METAL_BACKEND_HPP
#define GRAN_RHI_METAL_BACKEND_HPP

#include "gran/rhi/render_backend.hpp"

namespace gran::rhi
{
    /*
    ** Бэкенд Metal (macOS 11+, шаг M5): пока только вне экрана — кадр рисуется в свою текстуру и читается
    ** через SnapshotSource. Окно с CAMetalLayer появится с платформой Cocoa (M6); с nativeWindow бросает
    ** GpuException. Заголовок — чистый C++: Objective-C спрятан в файлах .mm из src/rhi/metal.
    **
    ** The Metal backend (macOS 11+, step M5): offscreen only for now — the frame is drawn into its own texture and read
    ** through SnapshotSource. A window with CAMetalLayer comes with the Cocoa platform (M6); with nativeWindow it throws
    ** GpuException. The header is plain C++: Objective-C is hidden in the .mm files of src/rhi/metal.
    */
    std::unique_ptr<RenderBackend> createMetalBackend(const BackendOptions& options);
}

#endif
