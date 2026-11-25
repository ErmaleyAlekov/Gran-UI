#ifndef GRAN_PLATFORM_INTERNAL_WEB_MESSAGE_LIMITS_HPP
#define GRAN_PLATFORM_INTERNAL_WEB_MESSAGE_LIMITS_HPP

#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran::platform
{
    // Пределы очереди и одной строки / Limits for a queue and one string
    constexpr std::size_t WEB_QUEUE_LIMIT = 256;
    constexpr std::size_t WEB_TEXT_LIMIT = 64 * 1024;

    inline bool webQueueAccepts(std::size_t count, std::size_t length) noexcept
    {
        return count < WEB_QUEUE_LIMIT && length <= WEB_TEXT_LIMIT;
    }

    inline void requireWebQueueSpace(std::size_t count, std::size_t length)
    {
        if (!webQueueAccepts(count, length))
            throw GranException(ErrorCode::LIMIT_EXCEEDED, i18n::format(i18n::Message::WEBVIEW_QUEUE_LIMIT));
    }
}

#endif
