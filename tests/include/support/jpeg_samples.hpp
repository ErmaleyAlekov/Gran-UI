#ifndef GRAN_TESTS_SUPPORT_JPEG_SAMPLES_HPP
#define GRAN_TESTS_SUPPORT_JPEG_SAMPLES_HPP

#include <cstddef>
#include <cstdint>

// Образцы JPEG для тестов декодера и эталонные пиксели RGB (декодирование libjpeg)
// The JPEG samples for the decoder tests and the reference RGB pixels (the libjpeg decoding)
namespace jpeg_samples
{
    // Размер всех образцов / The size of all the samples
    constexpr std::uint32_t WIDTH = 19;
    constexpr std::uint32_t HEIGHT = 13;

    extern const std::uint8_t BASELINE_420[];
    extern const std::size_t BASELINE_420_SIZE;
    extern const std::uint8_t BASELINE_420_RGB[];
    extern const std::size_t BASELINE_420_RGB_SIZE;
    extern const std::uint8_t PROGRESSIVE_420[];
    extern const std::size_t PROGRESSIVE_420_SIZE;
    extern const std::uint8_t PROGRESSIVE_420_RGB[];
    extern const std::size_t PROGRESSIVE_420_RGB_SIZE;
    extern const std::uint8_t BASELINE_444_RESTART[];
    extern const std::size_t BASELINE_444_RESTART_SIZE;
    extern const std::uint8_t BASELINE_444_RESTART_RGB[];
    extern const std::size_t BASELINE_444_RESTART_RGB_SIZE;
    extern const std::uint8_t PROGRESSIVE_GRAY[];
    extern const std::size_t PROGRESSIVE_GRAY_SIZE;
    extern const std::uint8_t PROGRESSIVE_GRAY_RGB[];
    extern const std::size_t PROGRESSIVE_GRAY_RGB_SIZE;
}

#endif
