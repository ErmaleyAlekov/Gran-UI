#ifndef GRAN_RENDER_RENDERER_HPP
#define GRAN_RENDER_RENDERER_HPP

#include "gran/render/content_sizer.hpp"
#include "gran/render/scene_builder.hpp"

#include <string>

namespace gran::render
{
    // Слот текстуры ещё не создан / The texture slot is not created yet
    constexpr std::uint32_t NO_TEXTURE = 0xFFFFFFFFu;

    // Текстура на GPU, повторяющая CPU-ресурс одного поколения / A GPU texture mirroring a CPU resource of one generation
    struct MirroredTexture
    {
        std::uint32_t slot = NO_TEXTURE;
        std::uint32_t generation = 0;
    };

    /*
    ** Рендерер окна (паттерн Facade): сцена из дерева, атлас глифов, рампы градиентов,
    ** их зеркала на GPU и отправка кадра бэкенду. Атлас или рампа, сменившие поколение
    ** во время построения сцены (рост, сброс), делают сцену устаревшей — она строится заново.
    **
    ** The window renderer (the Facade pattern): the scene from the tree, the glyph atlas, the gradient ramps,
    ** their GPU mirrors and submitting the frame to the backend. An atlas or ramp that changed its generation
    ** while the scene was built (growing, resetting) makes the scene stale — it is built again.
    */
    class Renderer
    {
    public:
        // ---- renderer.cpp ----
        // imageDirectory — папка, от которой отсчитываются относительные пути картинок
        // imageDirectory is the folder relative image paths are counted from
        Renderer(rhi::RenderBackend& backend, text::FontLibrary& fonts, const std::string& imageDirectory);
        ~Renderer();
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;

        // Измеритель текста для компоновки / The text measurer for layout
        TextEngine& textEngine() noexcept;

        // Измеритель содержимого (текст и картинки) для компоновки / The content measurer (text and images) for layout
        layout::ContentMeasurer& measurer() noexcept;

        // Кадр дерева со стилем окна; scale — DPI; caret — поле ввода с видимой кареткой, selection — узел
        // с видимым выделением (оба могут быть nullptr)
        // A frame of the tree with the window style; scale is the DPI; caret is the input with a visible caret, selection is
        // the node with a visible selection (both may be nullptr)
        rhi::FrameStatus render(const ui::Node& root, const style::ComputedStyle& window, float scale,
            const ui::Node* caret = nullptr, const ui::Node* selection = nullptr);

    private:
        // ---- renderer_textures.cpp ----
        // Атлас на GPU: при смене поколения — новая текстура, иначе — грязная область
        // The atlas on the GPU: on a generation change — a new texture, otherwise — the dirty area
        void syncAtlas();

        // Рампы на GPU / The ramps on the GPU
        void syncRamps();

        rhi::RenderBackend& _backend;
        TextEngine _text;
        ImageStore _images;
        ContentSizer _sizer;
        text::GlyphAtlas _atlas;
        GradientRamps _ramps;
        Scene _scene;
        MirroredTexture _atlasTexture;
        MirroredTexture _rampTexture;
    };
}

#endif
