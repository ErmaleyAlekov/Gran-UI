#include "gran/platform/headless/headless_window.hpp"
#include "gran/platform/internal/pixel_size.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <chrono>
#include <limits>

namespace gran::platform
{
    // Размер и место — из описания окна; масштаб 1 (тест меняет его через changeScale)
    // The size and place come from the window description; the scale is 1 (a test changes it via changeScale)
    HeadlessWindow::HeadlessWindow(const WindowDesc& desc)
        : _desc(desc), _width(desc.width), _height(desc.height)
    {
        const auto width = pixelExtent(desc.width);
        const auto height = pixelExtent(desc.height);
        if (width > std::numeric_limits<int>::max() || height > std::numeric_limits<int>::max())
            throw GranException(ErrorCode::INVALID_ARGUMENT,
                i18n::format(i18n::Message::STYLE_BAD_NUMBER, {std::to_string(width) + " x " + std::to_string(height)}));
        _state.placement.width = static_cast<int>(width);
        _state.placement.height = static_cast<int>(height);
    }

    // Очередь общая с потоком теста и фоновыми потоками, поэтому под мьютексом; ждущий просыпается
    // The queue is shared with the test thread and background threads, so it is under the mutex; the waiter wakes up
    void HeadlessWindow::pushEvent(const Event& event)
    {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _events.push_back(event);
        }
        _signal.notify_one();
    }

    // Первое событие очереди / The first queue event
    bool HeadlessWindow::pollEvent(Event& event)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (_events.empty())
            return false;
        event = _events.front();
        _events.pop_front();
        return true;
    }

    /*
    ** Сон до события, пробуждения или срока, как у окна ОС. Без срока (отрицательный) окно спит, пока его
    ** не разбудят: тест, который зовёт Application::run(), должен подать события или задать exitAfter.
    ** Цикл вокруг wait — от ложных пробуждений условной переменной.
    **
    ** Sleeping until an event, a wake or the timeout, as an OS window does. Without a timeout (negative) the window sleeps
    ** until woken: a test that calls Application::run() must feed events or set exitAfter.
    ** The loop around wait is against spurious wake-ups of the condition variable.
    */
    void HeadlessWindow::waitEvents(double timeout)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now()
            + std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(timeout));

        while (!_woken && _events.empty())
        {
            if (timeout < 0.0)
                _signal.wait(lock);
            else if (_signal.wait_until(lock, deadline) == std::cv_status::timeout)
                break;
        }
        _woken = false;
    }

    // Единственный метод для любого потока (контракт platform::) / The only method for any thread (the platform:: contract)
    void HeadlessWindow::wake() noexcept
    {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _woken = true;
        }
        _signal.notify_one();
    }
}
