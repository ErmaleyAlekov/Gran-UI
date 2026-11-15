#ifndef GRAN_RHI_METAL_BACKEND_HPP
#define GRAN_RHI_METAL_BACKEND_HPP

#include "gran/rhi/render_backend.hpp"

namespace gran::rhi
{
    /*
    ** Бэкенд Metal (macOS 11+, шаги M5 и M6): кадр рисуется в свою текстуру и читается через SnapshotSource;
    ** с nativeWindow (CAMetalLayer окна Cocoa, Window::nativeHandle) он ещё и показывается в окне. Не слой —
    ** GpuException. Заголовок — чистый C++: Objective-C спрятан в файлах .mm из src/rhi/metal.
    **
    ** The Metal backend (macOS 11+, steps M5 and M6): the frame is drawn into its own texture and read through
    ** SnapshotSource; with nativeWindow (the CAMetalLayer of a Cocoa window, Window::nativeHandle) it is also shown in the
    ** window. Not a layer — GpuException. The header is plain C++: Objective-C is hidden in the .mm files of src/rhi/metal.
    */
    std::unique_ptr<RenderBackend> createMetalBackend(const BackendOptions& options);
}

#endif
