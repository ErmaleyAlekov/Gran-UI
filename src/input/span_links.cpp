#include "gran/input/input_router.hpp"

namespace gran::input
{
    /*
    ** Участок-ссылка под точкой. У участков нет своих рамок (они части строк), поэтому попадание
    ** находит Text, а участок ищется по байту текста под мышью. Щелчок по правой половине
    ** последней буквы даёт байт конца участка — он тоже считается попаданием.
    **
    ** The link run under the point. The runs have no frames of their own (they are parts of lines), so the hit
    ** finds the Text, and the run is looked up by the text byte under the mouse. A click on the right half
    ** of the last letter gives the run end byte — it counts as a hit too.
    */
    ui::Node* spanAt(RouterState& state, ui::Node* hit, Vec2 point)
    {
        std::size_t begin = 0;

        if (hit == nullptr || state.text == nullptr || !ui::hasSpans(*hit))
            return hit;
        std::size_t offset = state.text->offsetAt(*hit, point);
        for (const std::unique_ptr<ui::Node>& span : hit->children)
        {
            std::size_t end = begin + span->content.text.size();
            bool inside = offset >= begin && (offset < end || (offset == end && end > begin));
            if (inside && !span->content.onClick.empty())
                return span.get();
            begin = end;
        }
        return hit;
    }
}
