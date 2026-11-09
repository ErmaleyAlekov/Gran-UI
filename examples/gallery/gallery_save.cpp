#include "gallery_actions.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace gallery
{
    /*
    ** Настройки диалога «Сохранить как…»: текстовый файл, чтобы пример действительно записал что-то
    ** понятное, а проверяющий открыл результат любым редактором.
    **
    ** The "Save as…" dialog options: a text file, so the sample really writes something readable,
    ** and the tester can open the result in any editor.
    */
    gran::FileDialogOptions saveSampleOptions()
    {
        gran::FileDialogOptions options;

        options.filters = {{"Текст", "*.txt"}, {"Все файлы", "*.*"}};
        options.fileName = "заметка Грани.txt";
        options.defaultExtension = "txt";
        return options;
    }

    /*
    ** Записывает в выбранный файл строку-образец и возвращает текст для строки состояния.
    ** Путь приходит из диалога в UTF-8: u8path нужен, чтобы русское имя не сломалось на Windows.
    **
    ** Writes a sample line into the chosen file and returns the text for the status line.
    ** The path comes from the dialog in UTF-8: u8path keeps a Russian name intact on Windows.
    */
    std::string saveSample(const std::string& path)
    {
        std::ofstream file(std::filesystem::u8path(path), std::ios::binary | std::ios::trunc);

        if (!file)
            return "Не удалось открыть для записи: " + path;
        file << "Грань UI: проверка сохранения файла / Gran UI: a file save check\n";
        file.close();
        if (!file)
            return "Не удалось записать: " + path;
        return "Сохранено: " + path;
    }
}
