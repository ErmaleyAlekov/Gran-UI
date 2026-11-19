#include "gran/app/internal/app_state.hpp"

namespace gran::app
{
    // Набранный символ начинает правку текущей ячейки (значение заменяется им) / A typed character starts editing the current cell (it replaces the value)
    bool AppWindow::listText(ui::Node& container, const std::string& text)
    {
        return beginGridEdit(*this, container, &text);
    }

    void AppWindow::listDoubleClick(ui::Node& container, ui::Node& hit)
    {
        gridDoubleClick(*this, container, hit);
    }

    bool AppWindow::editorKey(ui::Node& focus, const platform::Event& event)
    {
        return gridEditorKey(*this, focus, event);
    }
}
