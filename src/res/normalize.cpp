#include "gran/res/resources.hpp"

namespace gran::res
{
    namespace
    {
        // Добавить часть пути: «.» пропускается, «..» снимает предыдущую часть; false — выход за корень / Add a path part: "." is skipped, ".." removes the previous part; false — leaving the root
        bool pushPart(std::vector<std::string>& parts, const std::string& part)
        {
            if (part.empty() || part == ".")
                return true;
            if (part != "..")
            {
                parts.push_back(part);
                return true;
            }
            if (parts.empty())
                return false;
            parts.pop_back();
            return true;
        }
    }

    /*
    ** Склейки путей движка (std::filesystem на Windows) дают «res:/ui\assets/a.png» и «res:\ui\..\b.png»:
    ** обе косые считаются разделителями, приставка «res:» снимается, результат — «ui/assets/a.png».
    **
    ** The engine path joins (std::filesystem on Windows) give "res:/ui\assets/a.png" and "res:\ui\..\b.png":
    ** both slashes count as separators, the "res:" prefix is removed, the result is "ui/assets/a.png".
    */
    std::string normalize(std::string_view path)
    {
        std::vector<std::string> parts;
        std::string part;
        std::string result;

        if (path.compare(0, 4, "res:") == 0)
            path.remove_prefix(4);
        for (char symbol : path)
        {
            if (symbol != '/' && symbol != '\\')
            {
                part += symbol;
                continue;
            }
            if (!pushPart(parts, part))
                return std::string();
            part.clear();
        }
        if (!pushPart(parts, part))
            return std::string();
        for (const std::string& name : parts)
        {
            if (!result.empty())
                result += '/';
            result += name;
        }
        return result;
    }
}
