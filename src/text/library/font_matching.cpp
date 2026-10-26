#include "gran/text/font_library.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>

namespace gran::text
{
    namespace
    {
        // Расстояние: несовпадение курсива важнее любой разницы насыщенности / The distance: an italic mismatch outweighs any weight difference
        int candidateDistance(const FontCandidate& candidate, int weight, bool italic) noexcept
        {
            int distance = std::abs(candidate.weight - weight);

            if (candidate.italic != italic)
                distance += 10000;
            return distance;
        }

        // Ближайший кандидат среди уже известных / The nearest candidate among the already known ones
        FontCandidate* findBest(std::vector<FontCandidate>& candidates, const std::string& family, int weight, bool italic)
        {
            FontCandidate* best = nullptr;
            int bestDistance = 0;

            for (FontCandidate& candidate : candidates)
            {
                if (candidate.failed || !sameFamilyName(candidate.family, family))
                    continue;
                int distance = candidateDistance(candidate, weight, italic);
                if (best == nullptr || distance < bestDistance)
                {
                    best = &candidate;
                    bestDistance = distance;
                }
            }
            return best;
        }
    }

    /*
    ** Поиск в три ступени: уже известные кандидаты → известные файлы семейства (десяток
    ** файлов) → полное сканирование системных каталогов (сотни файлов, один раз).
    **
    ** A three-step search: the already known candidates → the known family files (a dozen
    ** files) → a full scan of the system directories (hundreds of files, once).
    */
    FontCandidate* FontLibrary::bestInFamily(const std::string& family, int weight, bool italic)
    {
        FontCandidate* best = findBest(_candidates, family, weight, italic);

        if (best == nullptr && probeKnown(family))
            best = findBest(_candidates, family, weight, italic);
        if (best == nullptr && !_scanned)
        {
            ensureScanned();
            best = findBest(_candidates, family, weight, italic);
        }
        return best;
    }

    // Известные файлы семейства читаются один раз; true — что-то добавлено / The known family files are read once; true — something was added
    bool FontLibrary::probeKnown(const std::string& family)
    {
        std::size_t before = _candidates.size();

        if (std::find(_probed.begin(), _probed.end(), family) != _probed.end())
            return false;
        _probed.push_back(family);
        for (const std::string& file : knownFontFiles(family))
        {
            for (const std::string& directory : systemFontDirectories())
            {
                std::error_code error;
                std::filesystem::path path = std::filesystem::path(directory) / file;
                if (std::filesystem::exists(path, error))
                    scanFontFile(path.string(), _candidates);
            }
        }
        return _candidates.size() > before;
    }

    // Файлы .ttf/.ttc/.otf в системных каталогах (рекурсивно, без исключений файловой системы)
    // The .ttf/.ttc/.otf files in the system directories (recursively, without file system exceptions)
    void FontLibrary::ensureScanned()
    {
        std::error_code error;

        _scanned = true;
        for (const std::string& directory : systemFontDirectories())
        {
            std::filesystem::recursive_directory_iterator walker(directory, error);
            for (; !error && walker != std::filesystem::recursive_directory_iterator(); walker.increment(error))
            {
                std::string extension = walker->path().extension().string();
                // tolower — через unsigned char: байты ≥ 0x80 (кириллица в путях) не должны быть отрицательными
                // tolower — through unsigned char: bytes ≥ 0x80 (Cyrillic in paths) must not be negative
                for (char& symbol : extension)
                    symbol = static_cast<char>(std::tolower(static_cast<unsigned char>(symbol)));
                if (extension == ".ttf" || extension == ".ttc" || extension == ".otf")
                    scanFontFile(walker->path().string(), _candidates);
            }
        }
    }
}
