#include "gran/app/app_driver.hpp"
#include "gran/ui/node.hpp"

namespace gran
{
    namespace
    {
        // Событие мыши в точке окна / A mouse event at a window point
        platform::Event pointerEvent(platform::EventKind kind, Vec2 position, platform::MouseButton button)
        {
            platform::Event event;

            event.kind = kind;
            event.position = position;
            event.button = button;
            return event;
        }
    }

    // frame — в координатах компоновки окна; nodeToWindow добавляет transform предков и самого узла
    // frame is in the window layout coordinates; nodeToWindow adds the transform of the ancestors and the node itself
    Vec2 AppDriver::centerOf(const ui::Node& node) const
    {
        const Rect& frame = node.layout.frame;

        return affineApply(ui::nodeToWindow(node), {frame.x + frame.width * 0.5f, frame.y + frame.height * 0.5f});
    }

    void AppDriver::hover(const ui::Node& node)
    {
        windowOf(node).pushEvent(pointerEvent(platform::EventKind::MOUSE_MOVE, centerOf(node),
            platform::MouseButton::LEFT));
        settle();
    }

    // Как пользователь: подвести мышь, нажать, отпустить — каждое событие в свой шаг цикла
    // As a user does: move the mouse, press, release — every event in its own loop step
    void AppDriver::click(const ui::Node& node, platform::MouseButton button)
    {
        platform::HeadlessWindow& target = windowOf(node);
        Vec2 point = centerOf(node);

        target.pushEvent(pointerEvent(platform::EventKind::MOUSE_MOVE, point, button));
        settle();
        target.pushEvent(pointerEvent(platform::EventKind::MOUSE_DOWN, point, button));
        settle();
        target.pushEvent(pointerEvent(platform::EventKind::MOUSE_UP, point, button));
        settle();
    }
}
