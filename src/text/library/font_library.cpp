#include "gran/text/font_library.hpp"

namespace gran::text
{
    namespace
    {
        // Список семейств через запятую, без пробелов и кавычек по краям / A comma separated family list without edge spaces and quotes
        std::vector<std::string> splitFamilies(const std::string& families)
        {
            std::vector<std::string> names;
            std::size_t start = 0;

            while (start <= families.size())
            {
                std::size_t comma = families.find(',', start);
                if (comma == std::string::npos)
                    comma = families.size();
                std::string name = families.substr(start, comma - start);
                std::size_t first = name.find_first_not_of(" \t\"'");
                std::size_t last = name.find_last_not_of(" \t\"'");
                if (first != std::string::npos)
                    names.push_back(name.substr(first, last - first + 1));
                start = comma + 1;
            }
            return names;
        }
    }

    // Признаки берутся сканированием, сам файл загрузится при выборе / The traits come from scanning, the file itself loads when chosen
    void FontLibrary::addFile(const std::string& path)
    {
        scanFontFile(path, _candidates);
    }

    // Семейства по порядку; обобщённые раскрываются в имена ОС; в конце — sans-serif
    // The families in order; the generic ones unfold into OS names; sans-serif at the end
    std::shared_ptr<const FontFace> FontLibrary::match(const std::string& families, int weight, bool italic)
    {
        std::vector<std::string> names = splitFamilies(families);

        names.push_back("sans-serif");
        for (const std::string& family : names)
        {
            std::vector<std::string> expanded = genericFamilies(family);
            if (expanded.empty())
                expanded.push_back(family);
            for (const std::string& name : expanded)
            {
                FontCandidate* candidate = bestInFamily(name, weight, italic);
                if (candidate != nullptr && loadCandidate(*candidate))
                    return candidate->face;
            }
        }
        return nullptr;
    }

    // Семейства замены по ОС: символы, CJK, индийские, ближневосточные письменности
    // The fallback families by OS: symbols, CJK, Indic, Middle Eastern scripts
    std::shared_ptr<const FontFace> FontLibrary::fallback(char32_t symbol, int weight, bool italic)
    {
#if defined(_WIN32)
        static const char* const FAMILIES[] = {"Segoe UI", "Segoe UI Symbol", "Microsoft YaHei", "Yu Gothic",
            "Malgun Gothic", "Nirmala UI", "Segoe UI Historic", "Ebrima", "Gadugi", "Arial"};
#elif defined(__APPLE__)
        static const char* const FAMILIES[] = {"Helvetica Neue", "PingFang SC", "Hiragino Sans", "Apple SD Gothic Neo",
            "Kohinoor Devanagari", "Geeza Pro", "Apple Symbols", "Arial Unicode MS"};
#else
        static const char* const FAMILIES[] = {"Noto Sans", "DejaVu Sans", "Noto Sans CJK SC", "Noto Sans Symbols",
            "Noto Sans Symbols2", "Noto Sans Devanagari", "Noto Sans Arabic", "Noto Sans Hebrew"};
#endif
        for (const char* family : FAMILIES)
        {
            FontCandidate* candidate = bestInFamily(family, weight, italic);
            if (candidate != nullptr && loadCandidate(*candidate) && candidate->face->glyphIndex(symbol) != 0)
                return candidate->face;
        }
        return nullptr;
    }
}
