#ifndef GRAN_EXAMPLES_SNAPSHOT_HPP
#define GRAN_EXAMPLES_SNAPSHOT_HPP

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
}

#endif
