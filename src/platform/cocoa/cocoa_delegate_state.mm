#include "gran/platform/cocoa/internal/cocoa_window_impl.hpp"

// Видимость окна: EXPOSE — «перерисовать», когда окно снова видно (контракт платформы)
// The window visibility: EXPOSE is "repaint" when the window is visible again (the platform contract)
@interface GranCocoaWindowDelegate (State)
- (void)windowDidChangeOcclusionState:(NSNotification*)notification;
- (void)windowDidDeminiaturize:(NSNotification*)notification;
- (void)windowDidEndLiveResize:(NSNotification*)notification;
@end

@implementation GranCocoaWindowDelegate (State)

// Окно снова не закрыто другими: кадр, пропущенный за невидимостью (OCCLUDED), рисуется заново
// The window is no longer covered by others: the frame skipped while invisible (OCCLUDED) is drawn again
- (void)windowDidChangeOcclusionState:(NSNotification*)notification
{
    NSWindow* window = notification.object;

    if (self.owner != nullptr && (window.occlusionState & NSWindowOcclusionStateVisible) != 0)
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
