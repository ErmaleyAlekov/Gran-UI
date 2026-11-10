#include "gran/layout/internal/layout_internal.hpp"

#include <algorithm>
#include <cmath>

namespace gran::layout::internal
{
    namespace
    {
        // Высота строки текста узла / The text line height of the node
        float lineHeightOf(const ui::Node& node) noexcept
        {
            return node.computed.fontSize * node.computed.lineHeight;
        }

        // Поле ввода: строка текста; ширина по умолчанию — 20 средних символов, как <input size=20>
        // An input: one text line; the default width is 20 average characters, as <input size=20>
        Size inputSize(const ui::Node& node) noexcept
        {
            return Size{node.computed.fontSize * 0.55f * 20.0f, lineHeightOf(node)};
        }

        /*
        ** Флажок, радиокнопка, переключатель: значок (квадрат, круг или дорожка с бегунком — те же
        ** пропорции, что у отрисовки в paint_controls.cpp и paint_toggles.cpp), промежуток и подпись.
        **
        ** A check box, a radio button, a switch: the mark (a square, a circle or a track with a knob — the same
        ** proportions as the painting in paint_controls.cpp and paint_toggles.cpp), a gap and the label.
        */
        Size toggleSize(ui::Node& node, Size contentAvailable, LayoutPass& pass)
        {
            float height = std::round(node.computed.fontSize * 1.15f);
            float width = height;
            float gap = std::round(node.computed.fontSize * 0.5f);
            Size label{0.0f, 0.0f};

            if (node.kind == ui::ElementKind::SWITCH)
                width = std::round(height * 1.8f);
            if (!node.content.text.empty())
                label = pass.measurer.measureText(node, std::max(0.0f, contentAvailable.width - width - gap));
            if (label.width <= 0.0f)
                gap = 0.0f;
            return Size{width + gap + label.width, std::max(height, label.height)};
        }

        // Картинка: при одной заданной стороне вторая — по пропорциям / An image: with one side set the other follows the proportions
        Size imageLeafSize(ui::Node& node, Size contentAvailable, LayoutPass& pass)
        {
            Size intrinsic = pass.measurer.imageSize(node);
            bool fixedWidth = hasFixedSize(node, true);
            bool fixedHeight = hasFixedSize(node, false);

            if (intrinsic.width <= 0.0f || intrinsic.height <= 0.0f || fixedWidth == fixedHeight)
                return intrinsic;
            if (fixedWidth && std::isfinite(contentAvailable.width))
                return Size{contentAvailable.width, intrinsic.height * contentAvailable.width / intrinsic.width};
            if (fixedHeight && std::isfinite(contentAvailable.height))
                return Size{intrinsic.width * contentAvailable.height / intrinsic.height, contentAvailable.height};
            return intrinsic;
        }
    }

    /*
    ** Собственные размеры листьев (полоса прогресса — тонкая дорожка в полстроки). Текст и картинку меряет внешний измеритель;
    ** картинка с одной заданной стороной сохраняет пропорции (как <img> в HTML).
    **
    ** The intrinsic leaf sizes (a progress bar is a thin track of half a line). Text and images are measured by the external measurer;
    ** an image with one side set keeps its proportions (as <img> in HTML).
    */
    Size measureLeaf(ui::Node& node, Size contentAvailable, LayoutPass& pass)
    {
        switch (node.kind)
        {
            case ui::ElementKind::TEXT:
            case ui::ElementKind::BUTTON:
            case ui::ElementKind::COMBO_BOX:
            case ui::ElementKind::TAB_HEADER:
                return pass.measurer.measureText(node, contentAvailable.width);
            case ui::ElementKind::MENU_ITEM:
                return menuItemSize(node, contentAvailable, pass);
            case ui::ElementKind::SEPARATOR:
            case ui::ElementKind::TOOL_MORE:
            case ui::ElementKind::SIZE_GRIP:
            case ui::ElementKind::SPLITTER_HANDLE:
                return toolLeafSize(node);
            case ui::ElementKind::INPUT:
                return inputSize(node);
            case ui::ElementKind::CHECKBOX:
            case ui::ElementKind::RADIO_BUTTON:
            case ui::ElementKind::SWITCH:
                return toggleSize(node, contentAvailable, pass);
            case ui::ElementKind::SLIDER:
                return Size{160.0f, std::round(node.computed.fontSize * 1.4f)};
            case ui::ElementKind::PROGRESS_BAR:
                return Size{160.0f, std::round(node.computed.fontSize * 0.5f)};
            case ui::ElementKind::IMAGE:
            case ui::ElementKind::ICON:
                return imageLeafSize(node, contentAvailable, pass);
            default:
                return Size{0.0f, 0.0f};
        }
    }
}
