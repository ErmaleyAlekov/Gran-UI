#include "gran/text/internal/font_parts.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <utility>

namespace gran::text
{
    /*
    ** Загрузка по шагам; любой шаг может бросить GranException(FONT). Объект создаётся
    ** через shared_ptr<const>: после загрузки он неизменяем и разделяется рендерерами.
    **
    ** Loading step by step; any step may throw GranException(FONT). The object is created
    ** through shared_ptr<const>: after loading it is immutable and shared by the renderers.
    */
    std::shared_ptr<const FontFace> FontFace::load(std::shared_ptr<const MappedFile> file, std::uint32_t index,
        const std::string& source)
    {
        std::shared_ptr<FontFace> face(new FontFace());

        // Без файла начертание не создаётся: все чтения идут через FontReader по _file
        // A face is not created without a file: all reads go through FontReader over _file
        if (!file)
            throw GranException(ErrorCode::FONT, i18n::format(i18n::Message::FONT_NOT_FOUND, {source}));
        face->_file = std::move(file);
        FaceBuilder::readDirectory(*face, index, source);
        FaceBuilder::readHeader(*face, source);
        FaceBuilder::readOs2(*face);
        FaceBuilder::selectCmap(*face, source);
        FaceBuilder::readNames(*face);
        FaceBuilder::collectPairSubtables(*face);
        return face;
    }

    // Метрики начертания / The face metrics
    const FontMetrics& FontFace::metrics() const noexcept
    {
        return _metrics;
    }

    // Имя семейства / The family name
    const std::string& FontFace::family() const noexcept
    {
        return _family;
    }

    // Имя начертания / The style name
    const std::string& FontFace::style() const noexcept
    {
        return _style;
    }
}
