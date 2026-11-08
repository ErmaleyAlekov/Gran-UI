#ifndef GRAN_TESTS_RESIZE_DUPLICATION_HPP
#define GRAN_TESTS_RESIZE_DUPLICATION_HPP

#include "resize_capture.hpp"

#include <atomic>
#include <memory>
#include <thread>

namespace render_tests
{
    // Итог наблюдения за кадрами монитора / The result of watching the monitor frames
    struct WatchResult
    {
        bool opened = false;
        int frames = 0;
        int badFrames = 0;
        int worstColumns = 0;
    };

    // Общее для потока захвата и потока окна: полоса задаётся до старта, итог читается после join
    // Shared by the capture thread and the window thread: the band is set before the start, the result is read after join
    struct WatchShared
    {
        RECT band{};
        std::atomic<bool> stop{false};
        WatchResult result;
    };

    /*
    ** Наблюдатель кадров монитора через Desktop Duplication (DXGI 1.2). В отличие от снимка GDI, который
    ** заставляет DWM собрать свежую композицию, дублирование отдаёт ровно те кадры, что ушли на монитор, —
    ** вместе с промежуточными, где окно уже выросло, а его кадр ещё не пришёл. Устройство D3D11 и
    ** дублирование создаются и уничтожаются в собственном потоке наблюдателя.
    **
    ** A watcher of the monitor frames through Desktop Duplication (DXGI 1.2). Unlike a GDI capture, which makes
    ** DWM build a fresh composition, duplication gives exactly the frames sent to the monitor — together with the
    ** intermediate ones where the window has already grown but its frame has not arrived yet. The D3D11 device and
    ** the duplication are created and destroyed in the watcher's own thread.
    */
    class FrameWatcher
    {
    public:
        // Начать наблюдение за полосой экрана (координаты рабочего стола) / Start watching a screen band (desktop coordinates)
        void start(const RECT& band);

        // Остановить и вернуть итог / Stop and return the result
        WatchResult stop();

    private:
        std::shared_ptr<WatchShared> _shared;
        std::thread _thread;
    };

    // Запуск потока наблюдателя (тело — в resize_duplication.cpp) / Starting the watcher thread (the body is in resize_duplication.cpp)
    void startWatchLoop(const std::shared_ptr<WatchShared>& shared, std::thread& thread);
}

#endif
