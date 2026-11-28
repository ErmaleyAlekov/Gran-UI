#include "gran/ui/internal/command_sequence.hpp"

#include <algorithm>
#include <unordered_set>

namespace gran::ui
{
    // Складываем размеры за каждую ссылку, сохраняя для общего поддерева один результат.
    // Count every reference, memoizing a shared subtree once. Saturate before arithmetic can overflow.
    std::string inspectCommandSequences(const std::vector<const Command*>& roots, const CommandLookup& lookup,
        SequenceSizes& sizes)
    {
        struct Frame { const Command* command; std::size_t next = 0, leaves = 0; };
        std::vector<Frame> path;
        std::unordered_set<const Command*> active;
        for (const auto* root : roots)
        {
            if (sizes.count(root)) continue;
            active.insert(root);
            path.push_back({root});
            while (!path.empty())
            {
                auto& frame = path.back();
                if (frame.next == frame.command->sequence.size())
                {
                    sizes.emplace(frame.command, frame.leaves);
                    active.erase(frame.command);
                    path.pop_back();
                    continue;
                }
                const auto* child = lookup(frame.command->sequence[frame.next]);
                if (child && !child->sequence.empty() && !sizes.count(child))
                {
                    if (!active.insert(child).second) return child->id;
                    path.push_back({child});
                    continue;
                }
                ++frame.next;
                if (child && child->enabled)
                    frame.leaves = std::min(MAX_SEQUENCE_STEPS + 1, frame.leaves + (child->sequence.empty() ? 1 : sizes.at(child)));
            }
        }
        return {};
    }
}
