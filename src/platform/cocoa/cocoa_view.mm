#include "gran/platform/cocoa/internal/cocoa_window_impl.hpp"

@implementation GranCocoaView

/*
** Вид со слоем: слой создаёт makeBackingLayer (CAMetalLayer). Перерисовку слоя AppKit не просит (Never) — кадр
** приносит бэкенд Metal; при увеличении окна старый кадр не растягивается, а прижимается к левому верхнему углу
** до следующего кадра.
**
** A layer-backed view: makeBackingLayer creates the layer (CAMetalLayer). AppKit does not ask to redraw the layer
** (Never) — the Metal backend brings the frame; when the window grows, the old frame is not stretched but sticks to
** the top left corner until the next frame.
*/
- (instancetype)initWithOwner:(gran::platform::cocoa::CocoaWindow*)owner frame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self == nil)
        return nil;
    _owner = owner;
    _markedSelection = NSMakeRange(0, 0);
    self.wantsLayer = YES;
    self.layerContentsRedrawPolicy = NSViewLayerContentsRedrawNever;
    self.layerContentsPlacement = NSViewLayerContentsPlacementTopLeft;
    // Слежение за мышью без кнопок (MOUSE_MOVE) и уходом за край (MOUSE_LEAVE), пока программа активна; область сама
    // следует за видимой частью вида (InVisibleRect). События проходят через sendEvent: окна (cocoa_mouse.mm)
    // Tracking the mouse without buttons (MOUSE_MOVE) and leaving over the edge (MOUSE_LEAVE) while the program is active;
    // the area follows the visible part of the view itself (InVisibleRect). The events go through the window sendEvent: (cocoa_mouse.mm)
    NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved
        | NSTrackingActiveInActiveApp | NSTrackingInVisibleRect;
    [self addTrackingArea:[[NSTrackingArea alloc] initWithRect:NSZeroRect options:options owner:self userInfo:nil]];
    return self;
}

// Слой вида — поверхность Metal; непрозрачный: окно под ним не смешивается / The view layer is the Metal surface; opaque: the window below is not blended
- (CALayer*)makeBackingLayer
{
    CAMetalLayer* layer = [CAMetalLayer layer];

    layer.opaque = YES;
    return layer;
}

// Клавиатура приходит в этот вид / The keyboard comes to this view
- (BOOL)acceptsFirstResponder
{
    return YES;
}

// Начало координат — левый верхний угол, как у событий движка / The origin is the top left corner, as for the engine events
- (BOOL)isFlipped
{
    return YES;
}

// Курсор интерфейса на всём виде; AppKit ставит его сам, когда указатель входит в вид
// The interface cursor over the whole view; AppKit sets it itself when the pointer enters the view
- (void)resetCursorRects
{
    if (_owner != nullptr)
        [self addCursorRect:self.bounds cursor:_owner->cursor()];
}

@end
