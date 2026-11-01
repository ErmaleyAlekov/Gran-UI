#include "gran/data/internal/source_readers.hpp"

#include <utility>

namespace gran::data::internal
{
    namespace
    {
        // Сигнал отмены изнутри обработчика SAX: разбор «Берёсты» прерывается исключением / The cancel signal from inside the SAX handler: the Beresta parsing is interrupted by an exception
        struct FetchCancelled
        {
        };
    }

    /*
    ** В рабочем потоке: полная порция ждёт, пока поток интерфейса её заберёт; отмена прерывает разбор.
    ** Без порции (разбор в потоке интерфейса) записи просто копятся.
    **
    ** In the worker thread: a full portion waits until the UI thread takes it; a cancel interrupts the parsing.
    ** Without a portion (parsing in the UI thread) the records simply accumulate.
    */
    void XmlListFetcher::record(TreeItem item)
    {
        std::unique_lock<std::mutex> lock(_mutex);

        if (_spec.fetch == 0)
        {
            _queue.push_back(std::move(item.record));
            return;
        }
        while (_queue.size() >= _spec.fetch && !_cancelled)
            _taken.wait(lock);
        if (_cancelled)
            throw FetchCancelled();
        _queue.push_back(std::move(item.record));
        if (_queue.size() >= _spec.fetch)
            _ready.notify_one();
    }

    // Исключение не должно покинуть поток: ошибка передаётся потоку интерфейса через поле / An exception must not leave the thread: the error goes to the UI thread through a field
    void XmlListFetcher::work()
    {
        std::unique_ptr<DataException> error;

        try
        {
            parseXmlSource(_spec, *this);
        }
        catch (const FetchCancelled&)
        {
        }
        catch (const DataException& failure)
        {
            error = std::make_unique<DataException>(failure);
        }
        catch (const std::exception& failure)
        {
            error = std::make_unique<DataException>(failure.what(), _spec.path, 0, 0);
        }
        std::lock_guard<std::mutex> lock(_mutex);
        _error = std::move(error);
        _finished = true;
        _ready.notify_one();
    }

    // Ошибка отдаётся после записей, разобранных до неё / The error is given after the records parsed before it
    std::vector<Record> XmlListFetcher::takePortion(std::unique_lock<std::mutex>& lock)
    {
        std::vector<Record> portion;

        while (_queue.size() < _spec.fetch && !_finished)
            _ready.wait(lock);
        portion.swap(_queue);
        _taken.notify_one();
        if (portion.empty() && _error != nullptr)
        {
            DataException failure = *_error;
            _error.reset();
            throw failure;
        }
        return portion;
    }
}
