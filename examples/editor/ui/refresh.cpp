#include "ui/controller.hpp"
#include "../core/messages.hpp"
#include "../core/text.hpp"
namespace editor
{
    // onChange вызывается также при undo/redo встроенного текстового поля.
    // onChange also fires for the text field's built-in undo/redo.
    void Controller::changed(gran::ui::Node& node)
    {
        if (auto* item = document(nodeDocument(node)))
            refreshDocument(*item);
        updateCursor();
    }

    // Dirty вычисляется по содержимому, поэтому undo до исходного текста снимает метку.
    // Dirty state compares content, so undoing to the original text clears the marker.
    void Controller::refreshDocument(Document& item)
    {
        std::string label = item.name;
        if (item.page->content.text != item.saved) label += "  •";
        item.label->content.text = label;
        item.label->content.tooltip = item.path;
        item.page->visible = item.id == active;
        app.setStyle(*item.tab, "editor-tab");
        if (item.id == active) app.setStyle(*item.tab, "editor-tab-active");
        gran::ui::markDirty(*item.label, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
        gran::ui::markDirty(*item.page, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
    }

    // Пустое состояние — полноценный результат закрытия последней вкладки.
    // The empty state is a valid result of closing the last tab.
    void Controller::refresh()
    {
        for (auto& item : documents) refreshDocument(item);
        show("emptyEditor", documents.empty());
        auto* current = document(active);
        if (!current)
        {
            setText("breadcrumb", message(Message::NO_DOCUMENT));
            setText("windowTitle", "Gran Editor");
            setText("languageStatus", "");
            setText("cursorStatus", "");
            return;
        }
        std::string path = current->path;
        if (path.empty()) path = current->name;
        setText("breadcrumb", path);
        setText("windowTitle", current->name + " — Gran Editor");
        setText("languageStatus", languageLabel(current->path));
        updateCursor();
    }

    // Небольшой таймер отслеживает также перемещение курсора без изменения текста.
    // A small timer also tracks cursor movement without text changes.
    void Controller::updateCursor()
    {
        auto* current = document(active);
        if (!current) return;
        setText("cursorStatus", cursorLabel(current->page->content.text, current->page->content.caret));
    }
}
