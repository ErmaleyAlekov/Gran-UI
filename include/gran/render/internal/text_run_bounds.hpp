#ifndef GRAN_RENDER_INTERNAL_TEXT_RUN_BOUNDS_HPP
#define GRAN_RENDER_INTERNAL_TEXT_RUN_BOUNDS_HPP

#include <cstddef>
#include <cstdint>


namespace gran::render::internal
{
    std::uint32_t textRunEnd(std::uint32_t begin, std::size_t length);
}

#endif
