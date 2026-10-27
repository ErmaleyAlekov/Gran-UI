#ifndef GRAN_EXAMPLES_SNAPSHOT_HPP
#define GRAN_EXAMPLES_SNAPSHOT_HPP

#include "gran/layout/layout.hpp"
#include "gran/markup/markup_loader.hpp"

#include <string>

namespace snapshot
{
    // Параметры запуска gran_snapshot / The gran_snapshot launch parameters
    struct Arguments
    {
        std::string markup;
        std::string output;
        float width = 0.0f;
        float height = 0.0f;
        float scale = 1.0f;
    };

    // Конвейер одного кадра и запись PNG (snapshot_render.cpp) / One frame pipeline and writing the PNG (snapshot_render.cpp)
    int renderSnapshot(const Arguments& arguments);

    // Списки из <Data>, стили и компоновка с подгонкой виртуальных списков (snapshot_lists.cpp) / The lists from <Data>, the styles and the layout with fitting the virtual lists (snapshot_lists.cpp)
    void layoutDocument(gran::markup::UiDocument& document, const gran::style::ComputedStyle& window, gran::Size size,
        gran::layout::ContentMeasurer& measurer);
}

#endif
