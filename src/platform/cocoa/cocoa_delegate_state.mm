#include "gran/platform/cocoa/internal/cocoa_window_impl.hpp"
#include "gran/rhi/metal/metal_backend.hpp"

// Видимость окна: EXPOSE — «перерисовать», когда окно снова видно (контракт платформы)
// The window visibility: EXPOSE is "repaint" when the window is visible again (the platform contract)
@interface GranCocoaWindowDelegate (State)
- (void)windowDidChangeOcclusionState:(NSNotification*)notification;
- (void)windowDidDeminiaturize:(NSNotification*)notification;
- (void)windowDidEndLiveResize:(NSNotification*)notification;
@end

@implementation GranCocoaWindowDelegate (State)

// Видимость сменилась: отметка на слое для бэкенда Metal (не ждать drawable у невидимого окна, ревью Т-51); окно
// снова видно — кадр, пропущенный за невидимостью (OCCLUDED), рисуется заново
// The visibility changed: a mark on the layer for the Metal backend (no waiting for a drawable of an invisible window,
// review T-51); the window is visible again — the frame skipped while invisible (OCCLUDED) is drawn again
- (void)windowDidChangeOcclusionState:(NSNotification*)notification
{
    NSWindow* window = notification.object;
    BOOL visible = (window.occlusionState & NSWindowOcclusionStateVisible) != 0;

    [window.contentView.layer setValue:@(!visible) forKey:@(gran::rhi::METAL_LAYER_OCCLUDED_KEY)];
    if (self.owner != nullptr && visible)
        self.owner->push(gran::platform::EventKind::EXPOSE);
}

- (void)windowDidDeminiaturize:(NSNotification*)notification
{
    (void)notification;
    if (self.owner != nullptr)
        self.owner->push(gran::platform::EventKind::EXPOSE);
}

// Последний размер после перетаскивания края (промежуточные уже нарисованы обратным вызовом setLiveResize)
// и обычный кадр в цикле программы
// The last size after dragging the edge (the intermediate ones are already drawn by the setLiveResize callback)
// and a usual frame in the program loop
- (void)windowDidEndLiveResize:(NSNotification*)notification
{
    (void)notification;
    if (self.owner == nullptr)
        return;
    self.owner->resized();
    self.owner->push(gran::platform::EventKind::EXPOSE);
}

@end

namespace gran::platform::cocoa
{
    // Якорь компоновки категории State (cocoa_window_impl.hpp) / The State category link anchor (cocoa_window_impl.hpp)
    void linkDelegateState() noexcept
    {
    }
}
