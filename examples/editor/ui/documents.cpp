#include "ui/controller.hpp"
#include "../core/messages.hpp"
#include "../core/text.hpp"
#include <functional>
namespace editor
{
    // Пустой документ не связан с файлом до первого сохранения.
    // An empty document has no file until its first save.
    void Controller::newDocument()
    {
        createDocument("", "");
    }

    // Одна страница и одна история undo на каждый документ.
    // Each document has its own page and undo history.
    void Controller::createDocument(const std::string& path, const std::string& text)
    {
        Document item;
        item.id = ++serial;
        item.path = path;
        item.name = fileLabel(path);
        if (path.empty())
            item.name = message(Message::UNTITLED) + std::to_string(serial);
        item.saved = text;
        buildDocument(item, text);
        documents.push_back(item);
        select(item.id);
    }

    // Кнопки выбора и закрытия — соседние, поэтому закрытие не выбирает вкладку.
    // Selection and close buttons are siblings, so closing does not select the tab.
    void Controller::buildDocument(Document& item, const std::string& text)
    {
        using gran::ui::ElementKind;
        item.tab = &add(*app.find("editorTabs"), ElementKind::STACK, "editor-tab", "");
        item.tab->id = "tab-" + std::to_string(item.id);
        item.label = &add(*item.tab, ElementKind::BUTTON, "tab-label", item.name);
        item.label->content.onClick = "selectTab";
        item.close = &add(*item.tab, ElementKind::BUTTON, "tab-close", "×");
        item.close->content.onClick = "closeTab";
        item.close->content.tooltip = message(Message::CLOSE);
        item.close->content.accessibleName = std::string(message(Message::CLOSE)) + " " + item.name;
        item.page = &add(*app.find("editorPages"), ElementKind::TEXT_AREA, "code-input", text);
        item.page->id = "document-" + std::to_string(item.id);
        item.page->content.onChange = "documentChanged";
        item.page->content.accessibleName = item.name;
        item.label->content.tooltip = item.path;
        app.addEventListener(*item.tab, "pointerDown",
            std::bind(&Controller::pointer, this, std::placeholders::_1, std::placeholders::_2), true);
    }

    // Переключение не пересоздаёт текстовое поле и не сбрасывает выделение.
    // Switching never recreates the text field or resets its selection.
    void Controller::select(unsigned id)
    {
        if (!document(id))
            return;
        active = id;
        refresh();
        app.focus(document(id)->page);
    }

    // Циклический переход работает и для одной вкладки.
    // Cyclic navigation also handles a single tab.
    void Controller::cycle(int direction)
    {
        if (documents.empty())
            return;
        for (std::size_t index = 0; index < documents.size(); ++index)
            if (documents[index].id == active)
            {
                auto next = (index + documents.size() + direction) % documents.size();
                select(documents[next].id);
                return;
            }
    }
}
