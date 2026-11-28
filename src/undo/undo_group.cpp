#include "gran/undo/undo_stack.hpp"

#include <exception>

namespace gran::undo
{
    namespace
    {
        class GroupStep final : public UndoStep
        {
        public:
            void undo() override;
            void redo() override;
            std::vector<std::unique_ptr<UndoStep>> steps;
            std::size_t position = 0;
        };

        // Сохраняем позицию после каждого успешного шага, чтобы повторить прерванную отмену.
        // Keep the position after each successful step so interrupted undo can resume.
        void GroupStep::undo()
        {
            while (position)
            {
                steps[position - 1]->undo();
                --position;
            }
        }

        // Воспроизводим только зарегистрированные обратимые шаги группы.
        // Replay only the reversible steps recorded in the group.
        void GroupStep::redo()
        {
            try
            {
                while (position < steps.size())
                {
                    steps[position]->redo();
                    ++position;
                }
            }
            catch (...)
            {
                undo();
                throw;
            }
        }
    }

    // При отказе компенсации обе операции доступны в одном слоте истории, даже с пределом 1.
    // Failed compensation leaves both directions available in one history slot, even with a limit of one.
    std::size_t UndoStack::pendingRedoProgress() const noexcept
    {
        if (_index >= _steps.size()) return 0;
        const auto* group = dynamic_cast<const GroupStep*>(_steps[_index].get());
        return group ? group->position : 0;
    }

    // Новая правка после частичного повтора сохраняет уже применённую часть как завершённый шаг.
    // A new edit after a partial redo retains the applied prefix as a completed history step.
    void UndoStack::commitRedoPrefix()
    {
        const auto position = pendingRedoProgress();
        if (!position) return;
        auto& group = static_cast<GroupStep&>(*_steps[_index]);
        group.steps.resize(position);
        const bool savedHere = _clean == _index && _cleanProgress == position;
        if (_clean > _index || (_clean == _index && _cleanProgress))
        {
            _clean = NO_CLEAN;
            _cleanProgress = 0;
        }
        ++_index;
        if (savedHere) _clean = _index;
        _merge = false;
    }

    // Уже выполненные шаги сохраняются одним блоком, даже если поздний обработчик сообщил ошибку.
    // Preserve completed steps as a single unit even if a later handler reports an error.
    void UndoStack::group(const std::function<void()>& action)
    {
        if (_group || _busy)
            return action();
        auto step = std::make_unique<GroupStep>();
        std::exception_ptr failure;
        _group = &step->steps;
        try { action(); }
        catch (...) { failure = std::current_exception(); }
        _group = nullptr;
        step->position = step->steps.size();
        if (step->position)
            push(std::move(step));
        if (failure)
            std::rethrow_exception(failure);
    }

    // Именованные стеки направляют новые шаги в активную группу своего приложения.
    // Named stacks direct new steps to the active group of their application.
    void UndoStack::setGroupTarget(UndoStack* const* target) noexcept
    {
        _groupTarget = target;
    }
}
