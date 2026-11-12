#include "gran/platform/x11/x11_window.hpp"
#include "gran/core/i18n.hpp"
#include "gran/log/log.hpp"
#include <string>

namespace gran::platform::x11
{
    namespace
    {
        // Имя атома для журнала; None — «None» / An atom name for the log; None — "None"
        std::string atomName(Display* display, Atom atom)
        {
            std::string name = "None";

            if (atom == None)
                return name;
            char* text = XGetAtomName(display, atom);
            if (text != nullptr)
            {
                name = text;
                XFree(text);
            }
            return name;
        }
    }

    void logDndAware(::Window window, long version)
    {
        log::write(log::Level::INFO, "x11",
            i18n::format(i18n::Message::LOG_XDND_AWARE, {std::to_string(window), std::to_string(version)}));
    }

    // Типы — через запятую, не больше 16 (у Nautilus их обычно 3–6); номера окон — десятичные, как принимает xprop -id
    // The types are comma-separated, at most 16 (Nautilus usually has 3–6); the window ids are decimal, as xprop -id accepts
    void logDndEnter(Display* display, const DndState& state, const std::vector<Atom>& types)
    {
        std::string names;
        std::string offered = "no";

        for (std::size_t index = 0; index < types.size() && index < 16; ++index)
        {
            if (!names.empty())
                names += ", ";
            names += atomName(display, types[index]);
        }
        if (state.uriList)
            offered = "yes";
        log::write(log::Level::INFO, "x11", i18n::format(i18n::Message::LOG_XDND_ENTER,
            {std::to_string(state.source), std::to_string(state.version), names, offered}));
    }

    void logDndDrop(const DndState& state)
    {
        std::string accepted = "no";

        if (state.accepted)
            accepted = "yes";
        log::write(log::Level::INFO, "x11", i18n::format(i18n::Message::LOG_XDND_DROP,
            {std::to_string(state.point.x), std::to_string(state.point.y), accepted}));
    }

    void logDndData(Display* display, Atom type, int format, unsigned long bytes, std::size_t paths)
    {
        log::write(log::Level::INFO, "x11", i18n::format(i18n::Message::LOG_XDND_DATA,
            {atomName(display, type), std::to_string(format), std::to_string(bytes), std::to_string(paths)}));
    }
}
