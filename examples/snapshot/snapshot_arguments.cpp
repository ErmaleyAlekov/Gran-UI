#include "snapshot.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace snapshot
{
    namespace
    {
        // strtof с проверкой всего аргумента и конечности значения.
        // strtof with full argument and finite value validation.
        float dimension(const char* text)
        {
            char* end = nullptr;
            float value = std::strtof(text, &end);

            if (end == text || *end != '\0' || !std::isfinite(value) || value <= 0 || value > 16384)
                throw gran::GranException(gran::ErrorCode::INVALID_ARGUMENT,
                    gran::i18n::format(gran::i18n::Message::STYLE_BAD_NUMBER, {text}));
            return value;
        }
    }

    // Ключи допускаются после путей вперемешку с прежними позиционными размерами.
    // Options may follow paths mixed with the former positional dimensions.
    Arguments parseArguments(int argc, char* argv[])
    {
        Arguments args;
        float* dimensions[] = {&args.width, &args.height, &args.scale};
        unsigned count = 0;

        args.markup = argv[1];
        args.output = argv[2];
        for (int index = 3; index < argc; ++index)
        {
            if (std::strcmp(argv[index], "--d3d11") == 0)
                args.backend = gran::rhi::BackendKind::D3D11;
            else if (std::strcmp(argv[index], "--d3d12") == 0)
                args.backend = gran::rhi::BackendKind::D3D12;
            else if (std::strcmp(argv[index], "--vulkan") == 0)
                args.backend = gran::rhi::BackendKind::VULKAN;
            else if (std::strcmp(argv[index], "--software") == 0)
                args.forceWarp = true;
            else if (std::strcmp(argv[index], "--warp") == 0)
                args.forceWarp = true;
            else if (count < 3)
                *dimensions[count++] = dimension(argv[index]);
            else
                dimension("invalid");
        }
        if (count == 1)
            dimension("invalid");
        return args;
    }
}
