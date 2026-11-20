#include "gran/input/input_router.hpp"

namespace gran::input
{
    // Без привязки списков показывать нечего / Without list bindings there is nothing to show
    bool ActionSink::listReveal(ui::Node&, std::size_t)
    {
        return false;
    }
}
