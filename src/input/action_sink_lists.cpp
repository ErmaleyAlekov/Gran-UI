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

    // Без приложения ни списков, ни правки ячеек нет / Without an application there are neither lists nor cell edits
    bool ActionSink::listText(ui::Node&, const std::string&)
    {
        return false;
    }

    void ActionSink::listDoubleClick(ui::Node&, ui::Node&)
    {
    }

    bool ActionSink::editorKey(ui::Node&, const platform::Event&)
    {
        return false;
    }
}
