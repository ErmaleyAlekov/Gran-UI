#include "gran/app/internal/app_state.hpp"

namespace gran::app
{
    /*
    ** Шрифты добавляются в общую библиотеку только в adoptDocument: её граням уже доверяют
    ** рендереры других окон, поэтому библиотеку нельзя заменить целиком. Здесь — лишь проверка
    ** (addFile молча пропускает испорченные файлы, load проверяет таблицы).
    **
    ** The fonts are added to the shared library only in adoptDocument: the renderers of other windows already rely
    ** on its faces, so the library cannot be replaced as a whole. Here — only a check
    ** (addFile silently skips damaged files, load validates the tables).
    */
    void validateFonts(const markup::UiDocument& document, const std::string& base)
    {
        for (const markup::FontFace& face : document.fonts)
        {
            std::string path = (std::filesystem::path(base) / face.source).lexically_normal().string();

            text::FontFace::load(MappedFile::open(path), 0, path);
        }
    }
}
