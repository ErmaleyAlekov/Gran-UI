#include "gran/data/internal/source_readers.hpp"
#include "gran/core/i18n.hpp"

#include <filesystem>
#include <utility>

namespace gran::data::internal
{
    // Путь select, поля и наличие файла проверяются сразу: ошибка видна при загрузке разметки / The select path, the fields and the file are checked at once: an error is visible when the markup loads
    XmlListFetcher::XmlListFetcher(SourceSpec spec)
        : _spec(std::move(spec))
    {
        std::error_code status;

        parseXmlPath(_spec.select);
        compileFields(_spec);
        if (!std::filesystem::is_regular_file(std::filesystem::u8path(_spec.path), status))
            throw GranException(ErrorCode::IO, i18n::format(i18n::Message::IO_READ_FAILED, {_spec.path}));
    }

    // Рабочий поток будится с признаком отмены: обработчик SAX бросает исключение и разбор заканчивается / The worker thread is woken with the cancel flag: the SAX handler throws and the parsing ends
    XmlListFetcher::~XmlListFetcher()
    {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _cancelled = true;
        }
        _taken.notify_all();
        if (_worker.joinable())
            _worker.join();
    }

    bool XmlListFetcher::hasMore() const
    {
        std::lock_guard<std::mutex> lock(_mutex);

        return !_finished || !_queue.empty() || _error != nullptr;
    }

    /*
    ** Без порции весь файл разбирается здесь же, в потоке интерфейса. С порцией первый вызов
    ** запускает рабочий поток (метод класса, без замыканий), каждый вызов ждёт свою порцию.
    **
    ** Without a portion the whole file is parsed right here, in the UI thread. With a portion the first call
    ** starts the worker thread (a class method, without closures), each call waits for its portion.
    */
    std::vector<Record> XmlListFetcher::next()
    {
        if (_spec.fetch == 0)
        {
            std::vector<Record> all;
            if (_finished)
                return all;
            _finished = true;
            parseXmlSource(_spec, *this);
            all.swap(_queue);
            return all;
        }
        std::unique_lock<std::mutex> lock(_mutex);
        if (!_started)
        {
            _started = true;
            _worker = std::thread(&XmlListFetcher::work, this);
        }
        return takePortion(lock);
    }
}
