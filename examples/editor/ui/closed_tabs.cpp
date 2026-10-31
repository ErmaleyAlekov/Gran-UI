#include "ui/controller.hpp"
#include "../core/messages.hpp"
#include <algorithm>
namespace editor
{
    // Удаление откладывается вызывающим обработчиком; фокус снимается заранее.
    // The caller defers removal; focus is cleared before deleting nodes.
    void Controller::removeDocument(unsigned id)
    {
        auto* item = document(id);
        if (!item) return;
        auto index = static_cast<std::size_t>(item - documents.data());
        Document history = *item;
        history.page = history.tab = history.label = history.close = nullptr;
        closed.push_back(std::move(history));
        if (closed.size() > 20) closed.pop_front();
        if (active == id) app.focus(nullptr);
        app.remove(*item->page);
        app.remove(*item->tab);
        documents.erase(documents.begin() + index);
        if (active == id)
        {
            active = 0;
            if (!documents.empty())
                active = documents[std::min(index, documents.size() - 1)].id;
        }
        refresh();
        if (auto* current = document(active)) app.focus(current->page);
    }

    // Отклонённые правки не воскресают: возвращается сохранённая версия.
    // Discarded edits do not reappear: reopening restores the saved version.
    void Controller::reopen()
    {
        if (closed.empty())
        {
            setText("statusMessage", message(Message::NO_CLOSED));
            return;
        }
        Document item = closed.back();
        closed.pop_back();
        if (!item.path.empty())
            openDocument(item.path);
        else
            createDocument("", item.saved);
    }
}
