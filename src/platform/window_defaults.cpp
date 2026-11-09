#include "gran/platform/window.hpp"

namespace gran::platform
{
    // Окно без перетаскивания файлов: зоны не нужны / A window without file dragging: the zones are not needed
    void Window::setDropZones(const std::vector<Rect>&)
    {
    }

    // Брошенных файлов не бывает / There are never dropped files
    std::vector<std::string> Window::takeDroppedFiles()
    {
        return std::vector<std::string>{};
    }

    // Окну без синхронизации с оконным менеджером отвечать некому / A window without window manager sync has no one to answer
    void Window::framePresented()
    {
    }

    // Никто не ждёт кадра / No one waits for the frame
    bool Window::waitsForFrame() const
    {
        return false;
    }
}
