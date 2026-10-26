#include "gran/text/font_library.hpp"
#include "gran/core/gran_exception.hpp"

namespace gran::text
{
    /*
    ** Отображение файла одно на путь: начертания TTC и повторные загрузки берут уже открытое.
    ** Библиотека хранит слабые ссылки — файл закрывается, когда его не держит ни одно начертание.
    **
    ** One mapping per path: TTC faces and repeated loads take the already opened one.
    ** The library keeps weak references — the file closes when no face holds it.
    */
    std::shared_ptr<const MappedFile> FontLibrary::openFile(const std::string& path)
    {
        std::shared_ptr<const MappedFile> file;

        for (OpenedFile& opened : _files)
        {
            if (opened.path != path)
                continue;
            file = opened.file.lock();
            if (!file)
            {
                file = MappedFile::open(path);
                opened.file = file;
            }
            return file;
        }
        file = MappedFile::open(path);
        _files.push_back(OpenedFile{path, file});
        return file;
    }

    // Загрузка один раз; испорченный или нечитаемый файл помечается и больше не выбирается
    // Loading once; a damaged or unreadable file is marked and is never chosen again
    std::shared_ptr<const FontFace> FontLibrary::loadCandidate(FontCandidate& candidate)
    {
        if (candidate.face || candidate.failed)
            return candidate.face;
        try
        {
            candidate.face = FontFace::load(openFile(candidate.path), candidate.index, candidate.path);
        }
        catch (const GranException&)
        {
            candidate.failed = true;
        }
        return candidate.face;
    }
}
