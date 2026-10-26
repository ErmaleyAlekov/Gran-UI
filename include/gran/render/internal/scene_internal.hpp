#ifndef GRAN_RENDER_INTERNAL_SCENE_INTERNAL_HPP
#define GRAN_RENDER_INTERNAL_SCENE_INTERNAL_HPP

#include "gran/render/scene_builder.hpp"

namespace gran::render::internal
{
    /*
    ** Состояние отрисовки ветви: преобразование логических координат окна в пиксели устройства,
    ** признак «только сдвиг и масштаб DPI» (тогда координаты выравниваются по пикселям),
    ** текущее отсечение и накопленная непрозрачность.
    **
    ** The paint state of a branch: the transform from logical window coordinates into device pixels,
    ** the "only a shift and the DPI scale" flag (then coordinates are aligned to pixels),
    ** the current clip and the accumulated opacity.
    */
    struct PaintState
    {
        Affine2D transform;
        bool snapped = true;
        std::uint32_t clip = 0;
        float opacity = 1.0f;
    };

    // Контекст обхода; layer — слой Popup, который рисуется сейчас (остальные слои обычный обход пропускает)
    // The walk context; layer is the Popup layer painted now (the plain walk skips the other layers)
    struct SceneContext
    {
        SceneResources& resources;
        Scene& scene;
        float scale;
        const ui::Node* layer = nullptr;
    };

    // Все глифы раскладки, без отбора по участку / All layout glyphs, without selecting by a run
    constexpr std::uint16_t ALL_RUNS = 0xFFFFu;

    // Как рисовать глифы: цвет, кегль растра, участок (ALL_RUNS — все) / How to draw glyphs: the color, the raster size, the run (ALL_RUNS — all)
    struct GlyphPaint
    {
        Color color;
        float size = 0.0f;
        std::uint16_t run = ALL_RUNS;
    };

    // Текст узла на месте: узел, его раскладка и левый верхний угол раскладки / The node text in place: the node, its layout and the layout top-left corner
    struct TextPlace
    {
        const ui::Node& node;
        const text::TextLayout& layout;
        Vec2 origin;
    };

    /*
    ** Заливка диапазона байтов показа [from, to) по строкам: полоса от top высотой height
    ** (0 — вся строка), радиус скругления, breaks — выделенный перевод строки виден полоской.
    **
    ** Filling the display byte range [from, to) by lines: a strip from top of height height
    ** (0 — the whole line), the corner radius, breaks — a selected line break shows as a strip.
    */
    struct RangeFill
    {
        std::size_t from = 0;
        std::size_t to = 0;
        Color color;
        float top = 0.0f;
        float height = 0.0f;
        float radius = 0.0f;
        bool breaks = false;
    };

    // ---- scene_builder.cpp ----

    // Узел и его поддерево / A node and its subtree
    void paintNode(const ui::Node& node, const PaintState& parent, SceneContext& context);

    // Открытые слои Popup поддерева поверх всего / The open Popup layers of the subtree over everything
    void paintPopups(const ui::Node& node, const PaintState& base, SceneContext& context);

    // ---- paint_state.cpp ----

    // Состояние для узла: его transform, непрозрачность / The state for the node: its transform, opacity
    PaintState nodeState(const ui::Node& node, const PaintState& parent);

    // Отсечение детей по внутреннему краю рамки (overflow: hidden, Scroll) / The children clip by the border inner edge
    PaintState clipChildren(const ui::Node& node, const PaintState& state, SceneContext& context);

    // ---- instance_place.cpp ----

    /*
    ** Экземпляр с местом: при выравнивании — прямоугольник в пикселях устройства и единичное
    ** преобразование (unit = scale), иначе — логический прямоугольник и transform (unit = 1).
    ** Размеры рамок, радиусов и теней умножаются на unit.
    **
    ** An instance with a place: when aligned — a rectangle in device pixels and the identity
    ** transform (unit = scale), otherwise — a logical rectangle and the transform (unit = 1).
    ** The border, radius and shadow sizes are multiplied by unit.
    */
    GpuInstance placeInstance(const PaintState& state, const Rect& logical, float scale, float& unit);

    // Радиусы, уменьшенные пропорционально, если не помещаются (правило CSS) / The radii reduced proportionally if they do not fit (the CSS rule)
    CornerRadii fitRadii(const CornerRadii& radii, float width, float height, float unit) noexcept;

    // Прямоугольник содержимого: рамка минус padding и border / The content rectangle: the frame minus padding and border
    Rect contentRect(const ui::Node& node) noexcept;

    // ---- paint_box.cpp ----

    // Внешние тени, фон с рамкой, внутренние тени / The outer shadows, the background with the border, the inner shadows
    void paintBox(const ui::Node& node, const PaintState& state, SceneContext& context);

    // ---- paint_fill.cpp ----

    // Заливка цветом или градиентом в экземпляр (геометрия градиента — в его координатах)
    // A color or gradient fill into an instance (the gradient geometry is in its coordinates)
    void applyPaint(GpuInstance& instance, const style::Paint& paint, SceneContext& context);

    // ---- paint_text.cpp ----

    // Текст узла в прямоугольнике содержимого / The node text in the content rectangle
    void paintText(const ui::Node& node, const PaintState& state, SceneContext& context);

    // Глифы раскладки (все или одного участка) начиная с точки origin / The layout glyphs (all or of one run) from the point origin
    void paintGlyphs(const text::TextLayout& layout, Vec2 origin, const GlyphPaint& paint, const PaintState& state,
        SceneContext& context);

    // ---- paint_caret.cpp ----

    // Место каретки (x от начала строки и номер строки) по байту показа caret / The caret place (x from the line start and the line number) by the display byte caret
    text::CaretStop caretStopAt(const text::TextLayout& layout, std::size_t caret, const std::string& shown) noexcept;

    // Каретка поля ввода: вертикальная черта высотой в строку / The input caret: a vertical bar of the line height
    void paintCaret(const ui::Node& node, Vec2 top, float height, const PaintState& state, SceneContext& context);

    // Каретка поля в месте байта content.caret (строка и x — по раскладке) / The field caret at the byte content.caret (the line and x — by the layout)
    void paintTextCaret(const TextPlace& place, const PaintState& state, SceneContext& context);

    // ---- text_place.cpp ----

    // Байт места каретки номер index (конец строки раскрывается в байт) / The byte of the caret place number index (a line end is resolved into a byte)
    std::size_t caretStopOffset(const text::TextLayout& layout, std::size_t index, const std::string& text) noexcept;

    // Левый верхний угол раскладки текста узла в прямоугольнике box / The top-left corner of the node text layout in the box rectangle
    Vec2 textOrigin(const ui::Node& node, const text::TextLayout& layout, const Rect& box);

    // ---- text_lines.cpp ----

    /*
    ** Место, куда можно поставить каретку при движении по строкам: не конец строки с мягким переносом
    ** (его байт — начало следующей строки, каретка ушла бы на неё).
    **
    ** A place where the caret may stand when moving by lines: not the end of a soft-wrapped line
    ** (its byte is the next line start, the caret would jump onto it).
    */
    bool usableStop(const text::TextLayout& layout, std::size_t index, const std::string& shown) noexcept;

    // Ближайшее по x пригодное место строки line (layout.carets.size() — строки нет) / The usable place of the line nearest by x (layout.carets.size() — no such line)
    std::size_t nearestStop(const text::TextLayout& layout, std::uint32_t line, float x, const std::string& shown) noexcept;

    // ---- paint_range.cpp ----

    // Прямоугольники под строками диапазона байтов показа / The rectangles under the lines of a display byte range
    void paintRange(const TextPlace& place, const RangeFill& fill, const PaintState& state, SceneContext& context);

    // Подсветка выделения узла под его текстом / The node selection highlight under its text
    void paintSelection(const TextPlace& place, const PaintState& state, SceneContext& context);

    // ---- paint_runs.cpp ----

    // Подложки участков (Span с background) / The run backgrounds (Spans with background)
    void paintRunBackgrounds(const TextPlace& place, const PaintState& state, SceneContext& context);

    // Глифы участков их цветом и кеглем и линии textDecoration / The run glyphs in their color and size and the textDecoration lines
    void paintRunGlyphs(const TextPlace& place, const PaintState& state, SceneContext& context);

    // ---- paint_backdrop.cpp ----

    // Размытая подложка под блоком с backdropBlur; начинает новый сегмент / A blurred backdrop under a box with backdropBlur; starts a new segment
    void paintBackdrop(const ui::Node& node, const PaintState& state, SceneContext& context);

    // Число экземпляров последнего сегмента / The instance count of the last segment
    void closeSegment(Scene& scene);

    // ---- paint_image.cpp ----

    // Картинка узла Image по imageFit в прямоугольнике содержимого / The Image node picture by imageFit in the content rectangle
    void paintImage(const ui::Node& node, const PaintState& state, SceneContext& context);

    // ---- paint_scrollbar.cpp ----

    // Тонкие полосы прокрутки поверх содержимого Scroll / Thin scrollbars over the Scroll content
    void paintScrollbars(const ui::Node& node, const PaintState& state, SceneContext& context);

    // ---- paint_controls.cpp ----

    // Флажок и ползунок / A check box and a slider
    void paintControl(const ui::Node& node, const PaintState& state, SceneContext& context);
}

#endif
