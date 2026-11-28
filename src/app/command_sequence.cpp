#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/ui/internal/command_sequence.hpp"

#include <set>

namespace gran::app
{
    namespace
    {
        void rejectCycle(const std::string& id)
        {
            throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::COMMAND_SEQUENCE_CYCLE, {id}));
        }

        struct SequenceBudget
        {
            ui::CommandLookup lookup;
            std::string rootId;
            std::size_t remaining = ui::MAX_SEQUENCE_STEPS;
            ui::SequenceSizes sizes;

            bool fits(const ui::Command& command)
            {
                std::size_t count = 1;
                if (!command.sequence.empty())
                {
                    if (!sizes.count(&command))
                    {
                        const auto cycle = ui::inspectCommandSequences({&command}, lookup, sizes);
                        if (!cycle.empty()) rejectCycle(cycle);
                    }
                    count = sizes.at(&command);
                }
                if (count > remaining)
                    throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::COMMAND_SEQUENCE_LIMIT,
                        {rootId, std::to_string(ui::MAX_SEQUENCE_STEPS)}));
                return count != 0;
            }
        };

        // Храним имена и копии списков, так как обработчик может заменить документ.
        // Keep names and owned lists because a handler can replace the document.
        void runSteps(AppWindow& win, const ui::Command& root, SequenceBudget& budget)
        {
            struct Frame { std::string id; std::vector<std::string> steps; std::size_t next = 0; };
            std::vector<Frame> path{{root.id, root.sequence}};
            std::set<std::string> active;
            while (!path.empty())
            {
                auto& frame = path.back();
                if (frame.next == frame.steps.size())
                {
                    active.erase(frame.id);
                    path.pop_back();
                    continue;
                }
                const auto* command = budget.lookup(frame.steps[frame.next++]);
                if (!command || !command->enabled || !budget.fits(*command)) continue;
                if (command->sequence.empty())
                {
                    --budget.remaining;
                    budget.sizes.clear();
                    runCommand(win, *command, *win.document.root);
                }
                else if (command->id == path.front().id && command->sequence == path.front().steps) rejectCycle(command->id);
                else if (active.insert(command->id).second) path.push_back({command->id, command->sequence});
                else rejectCycle(command->id);
            }
        }
    }

    void runSequence(AppWindow& win, const ui::Command& command)
    {
        auto& app = *win.shared.owner;
        auto& stack = app.undoStack();
        SequenceBudget budget{[&](const std::string& id) { return findCommand(win.shared, win, id); },
            command.id, ui::MAX_SEQUENCE_STEPS, {}};
        if (!budget.fits(command)) return;
        struct ResetTarget
        {
            UndoStack*& slot;
            UndoStack* previous;
            ~ResetTarget() { slot = previous; }
        } reset{win.shared.groupUndo, win.shared.groupUndo};
        win.shared.groupUndo = &stack;
        stack.group([&] { runSteps(win, command, budget); });
    }
}
