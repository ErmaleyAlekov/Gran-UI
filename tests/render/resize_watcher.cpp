#include "resize_duplication.hpp"

namespace render_tests
{
    // Полоса задаётся до запуска потока: дальше её читает только он / The band is set before the thread starts: then only it reads it
    void FrameWatcher::start(const RECT& band)
    {
        _shared = std::make_shared<WatchShared>();
        _shared->band = band;
        startWatchLoop(_shared, _thread);
    }

    // После join итог принадлежит снова потоку окна / After join the result belongs to the window thread again
    WatchResult FrameWatcher::stop()
    {
        _shared->stop.store(true);
        _thread.join();
        return _shared->result;
    }
}
