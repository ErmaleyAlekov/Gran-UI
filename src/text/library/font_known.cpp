#include "gran/text/font_library.hpp"

namespace gran::text
{
    namespace
    {
        // Семейство и его файлы / A family and its files
        struct KnownFamily
        {
            const char* family;
            const char* files[10];
        };

        // Распространённые семейства ОС: чтение десятка файлов вместо сотен при первом тексте
        // The common OS families: reading a dozen files instead of hundreds for the first text
#if defined(_WIN32)
        const KnownFamily KNOWN[] = {
            {"Segoe UI", {"segoeui.ttf", "segoeuib.ttf", "segoeuii.ttf", "segoeuiz.ttf", "seguisb.ttf", "seguisbi.ttf",
                "segoeuil.ttf", "seguili.ttf", "segoeuisl.ttf", "seguibl.ttf"}},
            {"Arial", {"arial.ttf", "arialbd.ttf", "ariali.ttf", "arialbi.ttf"}},
            {"Times New Roman", {"times.ttf", "timesbd.ttf", "timesi.ttf", "timesbi.ttf"}},
            {"Consolas", {"consola.ttf", "consolab.ttf", "consolai.ttf", "consolaz.ttf"}},
            {"Cascadia Mono", {"CascadiaMono.ttf"}},
            {"Segoe UI Symbol", {"seguisym.ttf"}},
            {"Microsoft YaHei", {"msyh.ttc", "msyhbd.ttc", "msyhl.ttc"}},
            {"Yu Gothic", {"YuGothR.ttc", "YuGothB.ttc", "YuGothM.ttc", "YuGothL.ttc"}},
            {"Malgun Gothic", {"malgun.ttf", "malgunbd.ttf"}},
            {"Nirmala UI", {"Nirmala.ttc"}}};
#else
        const KnownFamily KNOWN[] = {{"", {nullptr}}};
#endif
    }

    // Имена семейств сравниваются без учёта регистра ASCII / Family names are compared ignoring the ASCII case
    bool sameFamilyName(const std::string& first, const std::string& second) noexcept
    {
        if (first.size() != second.size())
            return false;
        for (std::size_t index = 0; index < first.size(); ++index)
        {
            char a = first[index];
            char b = second[index];
            if (a >= 'A' && a <= 'Z')
                a = static_cast<char>(a - 'A' + 'a');
            if (b >= 'A' && b <= 'Z')
                b = static_cast<char>(b - 'A' + 'a');
            if (a != b)
                return false;
        }
        return true;
    }

    // Файлы известного семейства / The files of a known family
    std::vector<std::string> knownFontFiles(const std::string& family)
    {
        std::vector<std::string> files;

        for (const KnownFamily& known : KNOWN)
        {
            if (!sameFamilyName(known.family, family))
                continue;
            for (const char* file : known.files)
            {
                if (file != nullptr)
                    files.push_back(file);
            }
        }
        return files;
    }

    // Обобщённые семейства по ОС; неизвестное имя — пустой список / The generic families by OS; an unknown name — an empty list
    std::vector<std::string> genericFamilies(const std::string& generic)
    {
#if defined(_WIN32)
        const char* sans[] = {"Segoe UI", "Arial"};
        const char* serif[] = {"Times New Roman", "Georgia"};
        const char* mono[] = {"Cascadia Mono", "Consolas"};
#elif defined(__APPLE__)
        const char* sans[] = {"SF Pro", "Helvetica Neue"};
        const char* serif[] = {"Times", "Georgia"};
        const char* mono[] = {"SF Mono", "Menlo"};
#else
        const char* sans[] = {"Noto Sans", "DejaVu Sans"};
        const char* serif[] = {"Noto Serif", "DejaVu Serif"};
        const char* mono[] = {"Noto Sans Mono", "DejaVu Sans Mono"};
#endif
        if (sameFamilyName(generic, "sans-serif") || sameFamilyName(generic, "system-ui"))
            return {sans[0], sans[1]};
        if (sameFamilyName(generic, "serif"))
            return {serif[0], serif[1]};
        if (sameFamilyName(generic, "monospace"))
            return {mono[0], mono[1]};
        return {};
    }
}
