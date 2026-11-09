#ifndef GRAN_RENDER_SCENE_BUILDER_HPP
#define GRAN_RENDER_SCENE_BUILDER_HPP

#include "gran/render/gradient_ramps.hpp"
#include "gran/render/image_store.hpp"
#include "gran/render/text_engine.hpp"
#include "gran/rhi/render_backend.hpp"
#include "gran/text/glyph_atlas.hpp"

#include <vector>

namespace gran::render
{
    /*
    ** Отметки ввода на кадре (объект параметров Renderer::render): поле с видимой кареткой, узел с видимым
    ** выделением текста (оба могут быть nullptr) и видны ли подчёркивания букв доступа меню.
    **
    ** The input marks on the frame (the Renderer::render parameter object): the input with a visible caret, the node with a visible
    ** text selection (both may be nullptr) and whether the menu access letter underlines are shown.
    */
    struct FrameMarks
    {
        const ui::Node* caret = nullptr;
        const ui::Node* selection = nullptr;
        bool mnemonics = false;
    };

    // Ресурсы, которые сцена наполняет по ходу обхода / The resources the scene fills while walking
    struct SceneResources
    {
        TextEngine& text;
        text::GlyphAtlas& atlas;
        GradientRamps& ramps;
        ImageStore& images;
        const ui::Node* caret = nullptr;
        // Узел с видимым выделением текста (или nullptr) / The node with a visible text selection (or nullptr)
        const ui::Node* selection = nullptr;
        // Подчёркивать букву доступа пунктов меню / Underline the access letter of the menu items
        bool mnemonics = false;
    };

    // Результат: экземпляры, отсечения (0 — «без отсечения»), сегменты, цвет фона
    // The result: the instances, the clips (0 — "no clip"), the segments, the background color
    struct Scene
    {
        std::vector<GpuInstance> instances;
        std::vector<GpuClip> clips;
        std::vector<rhi::FrameSegment> segments;
        Color clearColor;
    };

    /*
    ** Обход дерева в порядке отрисовки (дети по zIndex, при равенстве — по порядку) и упаковка
    ** в экземпляры GPU. scale — пиксели устройства на логический пиксель (DPI). Блоки без
    ** поворота и масштаба выравниваются по пикселям устройства — рамки в 1 px остаются чёткими.
    **
    ** Walking the tree in the paint order (children by zIndex, on a tie — by order) and packing
    ** into GPU instances. scale is device pixels per logical pixel (DPI). Boxes without
    ** rotation and scaling are aligned to device pixels — 1 px borders stay crisp.
    */
    void buildScene(const ui::Node& root, const style::ComputedStyle& window, float scale, SceneResources& resources,
        Scene& scene);
}

#endif
