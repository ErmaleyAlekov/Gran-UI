#pragma once

#include <cstdint>

namespace gran::platform
{
    // Размер в пикселях перед передачей платформе / Pixel extent before passing it to the platform
    std::uint32_t pixelExtent(double value);
}
