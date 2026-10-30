#include "gran/capi/internal/bridge.hpp"

namespace gran::capi
{
    namespace
    {
        /*
        ** Шаг из обратных вызовов языка: копия описания шага; release — когда стек отбросил шаг
        ** (деструктор), чтобы обёртка освободила свои ссылки на функции.
        **
        ** A step of language callbacks: a copy of the step description; release — when the stack dropped the step
        ** (the destructor), so the wrapper frees its references to the functions.
        */
        class CallbackStep final : public undo::UndoStep
        {
        public:
            explicit CallbackStep(const gran_undo_step& step);
            ~CallbackStep() override;
            void undo() override;
            void redo() override;

        private:
            gran_undo_step _step;
        };

        CallbackStep::CallbackStep(const gran_undo_step& step) : _step(step)
        {
        }

        CallbackStep::~CallbackStep()
        {
            if (_step.release != nullptr)
                _step.release(_step.user);
        }

        void CallbackStep::undo()
        {
            if (_step.undo != nullptr)
                _step.undo(_step.user);
        }

        void CallbackStep::redo()
        {
            if (_step.redo != nullptr)
                _step.redo(_step.user);
        }
    }

    std::unique_ptr<undo::UndoStep> callbackStep(const gran_undo_step& step)
    {
        return std::make_unique<CallbackStep>(step);
    }
}
