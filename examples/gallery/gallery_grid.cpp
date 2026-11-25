#include "gallery_events.hpp"

#include <cmath>
#include <cstdlib>

namespace gallery
{
    namespace
    {
        /*
        ** Проверка правки ячейки кодом программы (событие cellEdit у таблицы samples/grid.xml, onCellEdit="checkOrder").
        ** Значение уже приведено типом столбца (сумма — число с точкой), здесь — правила самой программы: клиент без
        ** пробелов по краям и не пустой, сумма от 1 до 10 000 000. Отказ — текст ошибки, он станет подсказкой у поля.
        **
        ** Checking a cell edit by the program code (the cellEdit event of the samples/grid.xml table, onCellEdit="checkOrder").
        ** The value is already normalized by the column type (the sum is a number with a dot), here are the program's own rules:
        ** the client without spaces at the edges and not empty, the sum from 1 to 10,000,000. A refusal is an error text, it becomes the input tooltip.
        */
        struct CheckOrder
        {
            gran::Application* application;

            void operator()(gran::ui::Node&, gran::ui::UiEvent& event) const
            {
                gran::ui::CellEdit& cell = event.cell;

                if (cell.field == "client")
                {
                    std::size_t first = cell.value.find_first_not_of(' ');
                    std::size_t last = cell.value.find_last_not_of(' ');
                    if (first == std::string::npos)
                        cell.error = "Клиент не может быть пустым";
                    else
                        cell.value = cell.value.substr(first, last - first + 1);
                }
                if (cell.field == "sum")
                {
                    char* end = nullptr;
                    const char* start = cell.value.c_str();
                    const double sum = std::strtod(start, &end);
                    if (end != start + cell.value.size() || !std::isfinite(sum) || sum < 1.0 || sum > 10000000.0)
                        cell.error = "Сумма — от 1 до 10 000 000";
                }
                if (cell.error.empty())
                    setText(*application, "editStatus", "Записано: " + cell.field + " = " + cell.value);
                else
                    setText(*application, "editStatus", cell.error);
            }
        };
    }

    // checkOrder — обработчик onCellEdit таблицы заказов / checkOrder is the onCellEdit handler of the orders table
    void registerGrid(gran::Application& application)
    {
        application.onEvent("checkOrder", CheckOrder{&application});
    }
}
