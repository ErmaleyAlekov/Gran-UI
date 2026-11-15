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

        // Ключ выбора API или программного адаптера; false — это не ключ бэкенда.
        // A key selecting the API or the software adapter; false — it is not a backend key.
        bool backendKey(const char* text, Arguments& args)
        {
            if (std::strcmp(text, "--d3d11") == 0)
                args.backend = gran::rhi::BackendKind::D3D11;
            else if (std::strcmp(text, "--d3d12") == 0)
                args.backend = gran::rhi::BackendKind::D3D12;
            else if (std::strcmp(text, "--vulkan") == 0)
                args.backend = gran::rhi::BackendKind::VULKAN;
            else if (std::strcmp(text, "--metal") == 0)
                args.backend = gran::rhi::BackendKind::METAL;
            else if (std::strcmp(text, "--software") == 0 || std::strcmp(text, "--warp") == 0)
                args.forceWarp = true;
            else
                return false;
            return true;
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
            if (backendKey(argv[index], args))
                continue;
            if (count < 3)
                *dimensions[count++] = dimension(argv[index]);
            else
                dimension("invalid");
        }
        if (count == 1)
            dimension("invalid");
        return args;
    }
}
