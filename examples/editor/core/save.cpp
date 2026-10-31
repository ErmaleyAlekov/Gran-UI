#include "document.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace editor
{
    // Атомарно заменяем файл только после успешной записи временного.
    // Replace the file atomically only after successfully writing the temporary file.
    static bool replaceFile(const std::filesystem::path& from, const std::filesystem::path& to)
    {
#ifdef _WIN32
        return MoveFileExW(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
        std::error_code error;
        std::filesystem::rename(from, to, error);
        return !error;
#endif
    }

    // Уникальная папка резервируется атомарно рядом с целевым файлом.
    // A unique directory is reserved atomically beside the destination file.
    bool writeDocument(const std::string& path, const std::string& text)
    {
        auto target = std::filesystem::u8path(path);
        auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        auto directory = target.parent_path() / (".gran-save-" + std::to_string(stamp));
        std::error_code error;
        if (!std::filesystem::create_directory(directory, error))
            return false;
        auto temporary = directory / "buffer";
        std::ofstream output(temporary, std::ios::binary);
        output.write(text.data(), static_cast<std::streamsize>(text.size()));
        output.close();
        bool success = false;
        if (output)
        {
            auto status = std::filesystem::status(target, error);
            if (!error && std::filesystem::exists(status))
                std::filesystem::permissions(temporary, status.permissions(), error);
            success = replaceFile(temporary, target);
        }
        std::filesystem::remove(temporary, error);
        std::filesystem::remove(directory, error);
        return success;
    }
}
