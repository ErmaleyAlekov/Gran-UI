#ifndef GRAN_UNDO_UNDO_STACK_HPP
#define GRAN_UNDO_UNDO_STACK_HPP

#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

namespace gran::undo
{
    /*
    ** Шаг отмены (паттерн Command, как QUndoCommand): умеет откатить себя и выполнить снова.
    ** В стек кладётся уже выполненное действие — push не вызывает redo (правка поля или код
    ** приложения сначала меняют данные, потом записывают шаг). Объединение: шаги с одинаковым
    ** неотрицательным mergeId стек предлагает верхнему шагу (mergeWith) — так набор символов
    ** в одном поле становится одним шагом.
    **
    ** An undo step (the Command pattern, as QUndoCommand): it can roll itself back and run again.
    ** The stack takes an action that is already done — push does not call redo (a field edit or the application
    ** code changes the data first, then records the step). Merging: the steps with the same
    ** non-negative mergeId are offered to the top step (mergeWith) — this way typing characters
    ** in one field becomes one step.
    */
    class UndoStep
    {
    public:
        // ---- undo_step.cpp ----
        virtual ~UndoStep();

        // Вернуть данные в состояние до шага / Return the data to the state before the step
        virtual void undo() = 0;

        // Выполнить шаг снова (после undo) / Run the step again (after undo)
        virtual void redo() = 0;

        // Вид для объединения; отрицательный (по умолчанию) — шаг не объединяется / The kind for merging; negative (the default) — the step does not merge
        virtual int mergeId() const noexcept;

        /*
        ** Вобрать следующий шаг того же mergeId (он уже выполнен); true — вобран, стек его не хранит.
        ** По умолчанию — false.
        **
        ** Absorb the next step of the same mergeId (it is already done); true — absorbed, the stack does not keep it.
        ** By default — false.
        */
        virtual bool mergeWith(const UndoStep& next);
    };

    class UndoStack;

    // Наблюдатель стека (паттерн Observer): вызывается после каждой смены состояния / A stack observer (the Observer pattern): called after every state change
    class UndoObserver
    {
    public:
        // ---- undo_step.cpp ----
        virtual ~UndoObserver();

        // Изменились шаги, позиция, «чистое» состояние или предел / The steps, the position, the clean state or the limit changed
        virtual void undoChanged(const UndoStack& stack) = 0;
    };

    // Шаг из двух обработчиков — для кода приложения и обёрток / A step of two handlers — for the application code and the wrappers
    class ActionStep final : public UndoStep
    {
    public:
        // ---- action_step.cpp ----
        ActionStep(std::function<void()> undoHandler, std::function<void()> redoHandler);
        void undo() override;
        void redo() override;

    private:
        std::function<void()> _undo;
        std::function<void()> _redo;
    };

    /*
    ** Стек отмены (как QUndoStack). Позиция index делит шаги: ниже — выполненные (их можно отменить),
    ** выше — отменённые (их можно повторить). Новый шаг отбрасывает отменённые. Объединение с верхним
    ** шагом идёт, пока его не прервали: отмена, повтор, setClean и breakMerge закрывают верхний шаг.
    ** «Чистое» состояние — позиция, где документ сохранён (setClean): isClean() == false — «* имя»
    ** в заголовке. Предел limit (0 — без предела) отбрасывает самые старые шаги; если «чистая» позиция
    ** ушла вместе с ними, вернуться в неё уже нельзя. Пока шаг выполняет undo/redo, стек занят:
    ** push, undo и redo из обработчика шага игнорируются (изменение — следствие отмены, а не новая правка).
    **
    ** An undo stack (as QUndoStack). The index position splits the steps: below are the done ones (they can be undone),
    ** above are the undone ones (they can be redone). A new step drops the undone ones. Merging with the top
    ** step goes on until interrupted: undo, redo, setClean and breakMerge close the top step.
    ** The clean state is the position where the document is saved (setClean): isClean() == false is "* name"
    ** in the title. The limit (0 — no limit) drops the oldest steps; if the clean position
    ** went away with them, it cannot be reached any more. While a step runs undo/redo, the stack is busy:
    ** push, undo and redo from a step handler are ignored (the change is a consequence of undo, not a new edit).
    */
    class UndoStack
    {
    public:
        // ---- undo_stack.cpp ----
        explicit UndoStack(std::size_t limit = 0);

        // Отменить шаг под позицией; false — нечего или стек занят / Undo the step below the position; false — nothing or the stack is busy
        bool undo();

        // Повторить шаг над позицией; false — нечего или стек занят / Redo the step above the position; false — nothing or the stack is busy
        bool redo();

        // Убрать все шаги; пустой стек «чистый» / Remove all the steps; an empty stack is clean
        void clear();

        // ---- undo_push.cpp ----

        // Записать выполненный шаг (или вобрать его в верхний); пустой указатель — ничего / Record a done step (or absorb it into the top one); a null pointer — nothing
        void push(std::unique_ptr<UndoStep> step);

        // Выполняет ли шаг сейчас undo/redo / Whether a step is running undo/redo now
        bool busy() const noexcept;

        // ---- undo_state.cpp ----
        bool canUndo() const noexcept;
        bool canRedo() const noexcept;

        // Позиция совпадает с сохранённой / The position matches the saved one
        bool isClean() const noexcept;

        // Текущая позиция — «чистая» (документ сохранён) / The current position is clean (the document is saved)
        void setClean();

        // Следующий шаг не объединяется с верхним (смена поля, перемещение каретки) / The next step does not merge with the top one (a field change, a caret move)
        void breakMerge() noexcept;

        // ---- undo_info.cpp ----

        // Число шагов (выполненных и отменённых) / The step count (done and undone)
        std::size_t count() const noexcept;

        // Число выполненных шагов (позиция) / The done step count (the position)
        std::size_t index() const noexcept;

        std::size_t limit() const noexcept;

        // Новый предел; лишние старые шаги отбрасываются сразу / A new limit; the extra old steps are dropped at once
        void setLimit(std::size_t limit);

        // Наблюдатель (один; nullptr — снять); стек им не владеет / The observer (one; nullptr — remove); the stack does not own it
        void setObserver(UndoObserver* observer) noexcept;

    private:
        // ---- undo_stack.cpp ----
        void notify();

        // ---- undo_push.cpp ----
        void dropUndone();
        bool tryMerge(const UndoStep& step);
        void trim();

        // «Чистой» позиции нет (она отброшена пределом или новым шагом поверх отменённых) / There is no clean position (dropped by the limit or by a new step over undone ones)
        static constexpr std::size_t NO_CLEAN = static_cast<std::size_t>(-1);

        std::vector<std::unique_ptr<UndoStep>> _steps;
        std::size_t _index = 0;
        std::size_t _clean = 0;
        std::size_t _limit = 0;
        UndoObserver* _observer = nullptr;
        // Верхний шаг открыт для объединения / The top step is open for merging
        bool _merge = false;
        // Шаг выполняет undo/redo / A step is running undo/redo
        bool _busy = false;
    };
}

#endif
