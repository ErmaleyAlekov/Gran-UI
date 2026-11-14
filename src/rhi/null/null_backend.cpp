#include "gran/rhi/null/null_backend.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran::rhi
{
    void NullBackend::resize(std::uint32_t width, std::uint32_t height)
    {
        _stats.width = width;
        _stats.height = height;
    }

    /*
    ** Кадр «показан» всегда. Проверяется согласованность данных сцены, как это сделал бы GPU-бэкенд перед
    ** записью в буферы: указатели есть, если счётчики не нулевые, и участки не выходят за число экземпляров.
    **
    ** The frame is always "presented". The scene data consistency is checked, as a GPU backend would do before
    ** writing into the buffers: the pointers exist if the counters are not zero, and the segments stay within the instance count.
    */
    FrameStatus NullBackend::render(const FrameData& frame)
    {
        if ((frame.instanceCount != 0 && frame.instances == nullptr) || (frame.clipCount != 0 && frame.clips == nullptr)
            || (frame.segmentCount != 0 && frame.segments == nullptr))
            throw GranException(ErrorCode::INVALID_ARGUMENT,
                i18n::format(i18n::Message::GPU_BAD_FRAME, {"counters without arrays"}));
        for (std::uint32_t index = 0; index < frame.segmentCount; ++index)
        {
            const FrameSegment& segment = frame.segments[index];

            if (segment.first > frame.instanceCount || segment.count > frame.instanceCount - segment.first)
                throw GranException(ErrorCode::INVALID_ARGUMENT,
                    i18n::format(i18n::Message::GPU_BAD_FRAME, {"a segment outside the instances"}));
        }
        ++_stats.frames;
        _stats.lastInstances = frame.instanceCount;
        _stats.lastClips = frame.clipCount;
        return FrameStatus::PRESENTED;
    }

    // Отладочного слоя нет — сообщений тоже / There is no debug layer — no messages either
    std::vector<std::string> NullBackend::takeDiagnostics()
    {
        return std::vector<std::string>();
    }

    AdapterInfo NullBackend::adapterInfo() const
    {
        AdapterInfo info;

        info.api = "null";
        info.name = "NullBackend";
        info.software = true;
        return info;
    }

    const NullStats& NullBackend::stats() const noexcept
    {
        return _stats;
    }
}
