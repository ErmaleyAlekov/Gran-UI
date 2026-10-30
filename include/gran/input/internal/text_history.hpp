#ifndef GRAN_INPUT_INTERNAL_TEXT_HISTORY_HPP
#define GRAN_INPUT_INTERNAL_TEXT_HISTORY_HPP

#include "gran/input/input_router.hpp"
#include "gran/undo/undo_stack.hpp"

#include <chrono>
#include <cstdint>
#include <string>

namespace gran::input::internal
{
    /*
    ** Вид правки текста для объединения шагов: набор, удаление назад (Backspace), вперёд (Delete);
    ** прочее (вставка, вырезание, удаление выделения, новая строка) — всегда отдельный шаг.
    **
    ** The text edit kind for merging steps: typing, erasing backward (Backspace), forward (Delete);
    ** the rest (pasting, cutting, erasing the selection, a new line) is always a separate step.
    */
    enum class EditKind : std::uint8_t { TYPING, ERASE_BACK, ERASE_FORWARD, OTHER };

    // Правка: байты [from, to) заменяются на inserted (UTF-8, уже очищенный по виду поля) / An edit: the bytes [from, to) are replaced with inserted (UTF-8, already cleaned by the field kind)
    struct TextEdit
    {
        std::size_t from = 0;
        std::size_t to = 0;
        std::string inserted;
        EditKind kind = EditKind::OTHER;
    };

    // Пауза набора, после которой символы идут новым шагом / The typing pause after which the characters go as a new step
    constexpr std::chrono::milliseconds TYPING_PAUSE{1000};

    // Глубина истории одного поля / The history depth of one field
    constexpr std::size_t FIELD_UNDO_LIMIT = 100;

    /*
    ** Шаг правки поля хранит разницу, а не весь текст: позиция, удалённое, вставленное, каретка и якорь
    ** до правки (их возвращает отмена). Повтор ставит каретку за вставленным. Шаг живёт в истории
    ** своего поля (ui::NodeContent::history), поэтому ссылка на узел не висит.
    **
    ** A field edit step keeps the difference, not the whole text: the position, the removed, the inserted, the caret and the anchor
    ** before the edit (undo restores them). Redo puts the caret after the inserted part. The step lives in the history
    ** of its field (ui::NodeContent::history), so the node reference does not dangle.
    */
    class TextStep final : public undo::UndoStep
    {
    public:
        // ---- text_step.cpp ----

        // Снимок до правки: removed — заменяемые байты, каретка и якорь — из поля / A snapshot before the edit: removed is the replaced bytes, the caret and the anchor are from the field
        TextStep(ui::Node& field, const TextEdit& edit, std::string removed);
        void undo() override;
        void redo() override;
        int mergeId() const noexcept override;

        // ---- text_step_merge.cpp ----

        // Набор подряд, Backspace подряд, Delete подряд — без паузы и в одном месте / Typing in a row, Backspace in a row, Delete in a row — without a pause and in one place
        bool mergeWith(const undo::UndoStep& next) override;

    private:
        ui::Node& _field;
        std::size_t _position;
        std::string _removed;
        std::string _inserted;
        std::size_t _caretBefore;
        std::size_t _anchorBefore;
        EditKind _kind;
        // Время последней вобранной правки (пауза набора считается от неё) / The time of the last absorbed edit (the typing pause counts from it)
        std::chrono::steady_clock::time_point _time;
    };

    // ---- text_change.cpp ----

    /*
    ** Единственная точка правки текста поля: запись шага в историю, замена, каретка за вставленным,
    ** перекомпоновка и onChange. false — только чтение или пустая правка.
    **
    ** The only point of a field text edit: recording the step in the history, the replacement, the caret after the inserted part,
    ** relayout and onChange. false — read-only or an empty edit.
    */
    bool applyEdit(RouterState& state, ui::Node& input, const TextEdit& edit);

    // Следующая правка поля — новый шаг (каретку переставили) / The next field edit is a new step (the caret was moved)
    void breakTyping(ui::Node& node) noexcept;

    // История, согласованная с текстом; текст сменили в обход правки — история сброшена, nullptr / The history consistent with the text; the text was changed bypassing the editing — the history is reset, nullptr
    ui::TextHistory* currentHistory(ui::Node& input);

    // ---- text_insert.cpp ----

    // Набор символов с клавиатуры: шаги объединяются / Typing characters from the keyboard: the steps merge
    bool typeText(RouterState& state, ui::Node& input, const std::string& text);

    // ---- text_undo.cpp ----

    // Отмена и повтор правки поля (onChange — как при обычной правке); false — нечего / Undoing and redoing a field edit (onChange — as on an ordinary edit); false — nothing
    bool undoText(RouterState& state, ui::Node& input);
    bool redoText(RouterState& state, ui::Node& input);

    // Есть ли что отменить / повторить (для контекстного меню) / Whether there is something to undo / redo (for the context menu)
    bool canUndoText(ui::Node& input);
    bool canRedoText(ui::Node& input);
}

#endif
