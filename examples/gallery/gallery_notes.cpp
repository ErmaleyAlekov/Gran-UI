#include "gallery_events.hpp"

#include <string>

namespace gallery
{
    namespace
    {
        /*
        ** Видимость заметки по id (а не по указателю): шаг отмены переживает перезагрузку разметки, узлы — нет.
        ** Шаг «удалить» прячет заметку, его отмена показывает её снова.
        **
        ** A note visibility by id (not by a pointer): an undo step survives a markup reload, the nodes do not.
        ** The "delete" step hides the note, its undo shows it again.
        */
        struct ShowNote
        {
            gran::Application* application;
            std::string id;
            bool visible;

            void operator()() const
            {
                gran::ui::Node* note = application->find(id);

                if (note == nullptr)
                    return;
                note->visible = visible;
                gran::ui::markDirty(*note, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
                if (visible)
                    setText(*application, "status", "Заметка «" + note->content.text + "» возвращена");
                else
                    setText(*application, "status", "Заметка «" + note->content.text + "» удалена (Ctrl+Z — вернуть)");
            }
        };

        // Del на заметке: узел исполнителя — заметка в фокусе (область focus внутри notes) / Del on a note: the running node is the focused note (the focus scope inside notes)
        struct DeleteNote
        {
            gran::Application* application;

            void operator()(gran::ui::Node& node) const
            {
                if (node.kind != gran::ui::ElementKind::BUTTON || node.parent == nullptr || node.parent->id != "notes")
                    return;
                // Скрытая заметка не должна держать фокус / A hidden note must not keep the focus
                application->focus(nullptr);
                ShowNote hide{application, node.id, false};
                hide();
                application->pushUndo(ShowNote{application, node.id, true}, hide);
            }
        };

        // «*» перед заголовком, пока позиция стека не совпадает с сохранённой / "*" before the title while the stack position differs from the saved one
        struct MarkModified
        {
            gran::Application* application;

            void operator()(const std::string&, const gran::UndoStack& stack) const
            {
                if (stack.isClean())
                    setText(*application, "docTitle", "Заметки");
                else
                    setText(*application, "docTitle", "* Заметки");
            }
        };
    }

    void registerNotes(gran::Application& application)
    {
        application.onAction("cmdDelete", DeleteNote{&application});
        application.onUndoChanged(MarkModified{&application});
    }
}
