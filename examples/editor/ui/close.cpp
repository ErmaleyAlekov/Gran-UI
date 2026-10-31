#include "ui/controller.hpp"
#include "../core/messages.hpp"
#include <algorithm>
namespace editor
{
    // Одиночное закрытие проходит тот же маршрут, что и закрытие окна.
    // Closing one tab uses the same route as closing the window.
    void Controller::requestClose(unsigned id)
    {
        if (pending || !document(id)) return;
        closeQueue.clear();
        closeQueue.push_back(id);
        continueClose();
    }

    // Очередь запрашивает решение отдельно для каждого изменённого документа.
    // The queue asks for a decision on each modified document separately.
    void Controller::closeAll(bool window)
    {
        if (pending) return;
        closeQueue.clear();
        closingWindow = window;
        for (const auto& item : documents)
            closeQueue.push_back(item.id);
        continueClose();
    }

    // Диалог нельзя случайно закрыть Escape, оставив очередь зависшей.
    // Escape cannot accidentally dismiss the dialog and leave the queue stuck.
    void Controller::continueClose()
    {
        while (!closeQueue.empty())
        {
            unsigned id = closeQueue.front();
            closeQueue.pop_front();
            auto* item = document(id);
            if (!item) continue;
            if (item->page->content.text != item->saved)
            {
                pending = id;
                select(id);
                setText("savePromptText", message(Message::SAVE_QUESTION) + item->name);
                auto* dialog = app.find("savePrompt");
                dialog->content.closable = false;
                app.showDialog(*dialog);
                return;
            }
            removeDocument(id);
        }
        if (closingWindow)
            app.closeWindow(*app.find("editorPages"));
        closingWindow = false;
    }

    // Ошибка сохранения или отмена сохраняют документ и прекращают закрытие.
    // A failed save or cancellation keeps the document and stops closing.
    void Controller::answerClose(const std::string& action)
    {
        if (!pending) return;
        unsigned id = pending;
        bool cancel = action == "cancelClose";
        app.hideDialog(*app.find("savePrompt"));
        if (action == "confirmSave" && !saveDocument(id, false))
            cancel = true;
        pending = 0;
        if (cancel)
        {
            closeQueue.clear();
            closingWindow = false;
            select(id);
            return;
        }
        removeDocument(id);
        continueClose();
    }
}
