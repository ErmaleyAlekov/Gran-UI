#include "gran/input/input_router.hpp"

namespace gran::input
{
    // Приёмник без окон (тесты, встроенные слои) панели не отрывает / A sink without windows (tests, built-in layers) does not tear panels off
    void ActionSink::floatDockPanel(ui::Node&)
    {
    }
}
