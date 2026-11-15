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

    /*
    ** Ключ KVC у CAMetalLayer окна: YES — окно свёрнуто или закрыто другими (ставит платформа Cocoa по уведомлению ОС),
    ** бэкенд тогда пропускает показ кадра без ожидания drawable (FrameStatus::OCCLUDED).
    **
    ** A KVC key of the window CAMetalLayer: YES — the window is minimized or covered by others (the Cocoa platform sets it by
    ** the OS notification), the backend then skips showing the frame without waiting for a drawable (FrameStatus::OCCLUDED).
    */
    constexpr const char* METAL_LAYER_OCCLUDED_KEY = "granOccluded";
}

#endif
