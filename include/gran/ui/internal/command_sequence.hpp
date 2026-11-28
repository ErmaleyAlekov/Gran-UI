#ifndef GRAN_UI_INTERNAL_COMMAND_SEQUENCE_HPP
#define GRAN_UI_INTERNAL_COMMAND_SEQUENCE_HPP

#include "gran/ui/command.hpp"
#include <functional>
#include <unordered_map>

namespace gran::ui
{
    using CommandLookup = std::function<const Command*(const std::string&)>;
    constexpr std::size_t MAX_SEQUENCE_STEPS = 1024;
    using SequenceSizes = std::unordered_map<const Command*, std::size_t>;
    std::string inspectCommandSequences(const std::vector<const Command*>& roots, const CommandLookup& lookup,
        SequenceSizes& sizes);
}

#endif
