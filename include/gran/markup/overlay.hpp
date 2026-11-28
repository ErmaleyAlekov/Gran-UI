#ifndef GRAN_MARKUP_OVERLAY_HPP
#define GRAN_MARKUP_OVERLAY_HPP

#include "gran/markup/markup_loader.hpp"

namespace gran::markup
{
    // Предел действует до копирования и разбора файла / Limit applied before copying or parsing the file
    inline constexpr std::size_t MAX_OVERLAY_BYTES = 4 * 1024 * 1024;
    void checkOverlaySize(std::size_t bytes);
    // База не меняется; результат можно опубликовать после успешной проверки всего профиля.
    // The base stays intact; publish the result after the entire profile passes validation.
    struct OverlayResult
    {
        UiDocument document;
        std::vector<std::string> missingIds;
    };

    OverlayResult applyOverlay(const UiDocument& base, std::string_view overlay, const LoadOptions& options);
    std::string resetOverlayScope(const UiDocument& base, const UiDocument& current, std::string_view xml, const std::string& scope);
    std::string appendOverlayEdit(std::string_view xml, const std::string& operation, const TokenMap& attributes,
        const TokenMap& parameters = {});
}

#endif
