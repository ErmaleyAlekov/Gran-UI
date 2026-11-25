#include "gran/platform/internal/pixel_size.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <limits>

namespace gran::platform
{
    std::uint32_t pixelExtent(double value)
    {
        if (!(value >= 0 && value <= std::numeric_limits<std::uint32_t>::max()))
            throw GranException(ErrorCode::INVALID_ARGUMENT,
                i18n::format(i18n::Message::STYLE_BAD_NUMBER, {std::to_string(value)}));
        return static_cast<std::uint32_t>(value);
    }
}
