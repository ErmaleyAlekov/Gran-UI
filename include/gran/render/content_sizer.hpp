#ifndef GRAN_RENDER_CONTENT_SIZER_HPP
#define GRAN_RENDER_CONTENT_SIZER_HPP

#include "gran/layout/layout.hpp"
#include "gran/render/image_store.hpp"
#include "gran/render/text_engine.hpp"

namespace gran::render
{
    /*
    ** Измеритель содержимого для компоновки (паттерн Adapter): текст — через TextEngine,
    ** картинки — по собственному размеру из ImageStore (загрузка при первом измерении).
    **
    ** The content measurer for layout (the Adapter pattern): text — through TextEngine,
    ** images — by the intrinsic size from ImageStore (loading on the first measurement).
    */
    class ContentSizer : public layout::ContentMeasurer
    {
    public:
        // ---- content_sizer.cpp ----
        ContentSizer(TextEngine& text, ImageStore& images);
        Size measureText(const ui::Node& node, float maxWidth) override;
        Size imageSize(const ui::Node& node) override;

    private:
        TextEngine& _text;
        ImageStore& _images;
    };
}

#endif
