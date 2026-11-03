#ifndef GRAN_UTILS_MAPPED_FILE_HPP
#define GRAN_UTILS_MAPPED_FILE_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace gran
{
    /*
    ** Файл, отображённый в память только для чтения. Страницы подгружает ОС по первому
    ** обращению и может выгрузить обратно без записи в файл подкачки: такая память не входит
    ** в частную память процесса и делится с другими процессами, открывшими тот же файл.
    ** Пока объект жив, файл нельзя удалить (Windows); усечение файла другим процессом на POSIX
    ** даёт SIGBUS — поэтому отображаются только системные и встроенные в приложение файлы.
    **
    ** A file mapped into memory read-only. The OS loads the pages on first access and may drop them
    ** without writing to the page file: this memory is not part of the process private memory
    ** and is shared with other processes that opened the same file.
    ** While the object lives the file cannot be deleted (Windows); truncating the file by another process
    ** on POSIX gives SIGBUS — so only system files and files shipped with the application are mapped.
    */
    class MappedFile
    {
    public:
        // ---- mapped_file.cpp ----

        // Отображение файла path; nullptr — файл не открыт или пуст / Mapping the path file; nullptr — the file is not opened or empty
        static std::shared_ptr<const MappedFile> open(const std::string& path);

        ~MappedFile();
        MappedFile(const MappedFile&) = delete;
        MappedFile& operator=(const MappedFile&) = delete;

        // Байты файла / The file bytes
        const std::uint8_t* data() const noexcept;

        // Размер файла в байтах / The file size in bytes
        std::size_t size() const noexcept;

    private:
        MappedFile() = default;

        // ---- mapped_file_os.cpp ----

        // Отображение средствами ОС; false — не удалось / Mapping by the OS means; false — failed
        bool map(const std::string& path);

        // Снятие отображения (идемпотентно) / Removing the mapping (idempotent)
        void unmap() noexcept;

        const std::uint8_t* _data = nullptr;
        std::size_t _size = 0;
        // Дескриптор объекта отображения Windows (на POSIX не нужен) / The Windows mapping object handle (not needed on POSIX)
        void* _mapping = nullptr;
        // Байты ресурса res:/ (не отображение: снимать нечего) / The res:/ resource bytes (not a mapping: nothing to remove)
        bool _resource = false;
    };
}

#endif
