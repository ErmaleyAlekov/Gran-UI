#include "gran/core/error_code.hpp"

namespace gran
{
    namespace
    {
        // Имена кодов в порядке перечисления ErrorCode
        // The code names in the order of the ErrorCode enumeration
        const char* const ERROR_CODE_NAMES[ERROR_CODE_COUNT] = {
            "MARKUP", "STYLE_VALUE", "RESOURCE", "FONT", "IMAGE", "GPU",
            "DEVICE_LOST", "PLATFORM", "IO", "INVALID_ARGUMENT", "LIMIT_EXCEEDED"
        };
    }

    // Имя кода; для значения вне перечисления — "UNKNOWN"
    // The code name; "UNKNOWN" for a value outside the enumeration
    const char* errorCodeName(ErrorCode code) noexcept
    {
        int index = static_cast<int>(code);

        if (index < 0 || index >= ERROR_CODE_COUNT)
            return "UNKNOWN";
        return ERROR_CODE_NAMES[index];
    }
}
