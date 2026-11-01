#include "gran/input/input_router.hpp"

namespace gran::input
{
    // Без приложения выделять некому / Without an application there is nobody to select
    void ActionSink::listPress(ui::Node&, ui::Node&, std::uint8_t)
    {
    }

    bool ActionSink::listKey(ui::Node&, platform::Key, std::uint8_t)
    {
        return false;
    }
}
