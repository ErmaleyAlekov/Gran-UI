#ifndef GRAN_DATA_LIST_MODEL_HPP
#define GRAN_DATA_LIST_MODEL_HPP

#include "gran/data/list_source.hpp"

#include <memory>

namespace gran::data
{
    /*
    ** Подгрузчик записей для ленивой загрузки (паттерн «Стратегия»): файл JSON/XML, обёртка, код
    ** приложения. next() возвращает следующую порцию; подгрузчик из кода может вместо этого сам
    ** вставить записи в модель и вернуть пустую порцию.
    **
    ** A record fetcher for lazy loading (the Strategy pattern): a JSON/XML file, a wrapper, the application
    ** code. next() returns the next portion; a fetcher from code may instead insert the records into the model
    ** itself and return an empty portion.
    */
    class ListFetcher
    {
    public:
        virtual ~ListFetcher() = default;

        // Остались ли записи / Whether records remain
        virtual bool hasMore() const = 0;

        // Следующая порция записей / The next record portion
        virtual std::vector<Record> next() = 0;
    };

    /*
    ** Модель списка: упорядоченные записи, описание столбцов (таблица) и подписчики. Данные живут
    ** в ядре, а не в коде приложения: разметка описывает вид одной записи (<ItemTemplate>), модель —
    ** сами записи. Работает только в потоке интерфейса; фоновые потоки правят её через Application::post.
    **
    ** A list model: ordered records, the column description (a table) and subscribers. The data lives in the core,
    ** not in the application code: the markup describes the look of one record (<ItemTemplate>), the model —
    ** the records themselves. It works only in the UI thread; background threads edit it through Application::post.
    */
    class ListModel final : public ListSource
    {
    public:
        // ---- list_model.cpp ----

        std::size_t size() const noexcept override;
        const Record& at(std::size_t index) const override;

        // Вставить запись перед index (index == size() — в конец) / Insert a record before index (index == size() — at the end)
        void insert(std::size_t index, Record record);

        // Вставить пачку записей одним уведомлением / Insert a batch of records with one notification
        void insertAll(std::size_t index, std::vector<Record> records);

        // Добавить запись в конец / Append a record at the end
        void append(Record record);

        // ---- list_model_edit.cpp ----

        // Удалить запись / Remove a record
        void remove(std::size_t index);

        // Удалить count записей начиная с index / Remove count records starting at index
        void remove(std::size_t index, std::size_t count);

        // Сменить одно поле записи (подписчики получают changed) / Change one record field (the subscribers get changed)
        void set(std::size_t index, const std::string& field, std::string value) override;

        // Заменить запись целиком / Replace a record whole
        void setRecord(std::size_t index, Record record);

        // Переставить записи [from, from + count) так, чтобы они начинались с to / Move the records [from, from + count) so that they start at to
        void move(std::size_t from, std::size_t count, std::size_t to);

        // ---- list_model_columns.cpp ----

        // Удалить все записи (подгрузчик снимается) / Remove all records (the fetcher is dropped)
        void clear();

        // Описание столбцов таблицы / The table column description
        void setColumns(ColumnSet columns);
        const ColumnSet& columns() const noexcept override;

        // Подгрузчик следующих порций (nullptr — снять) / The fetcher of the next portions (nullptr — drop)
        void setFetcher(std::shared_ptr<ListFetcher> fetcher);

        // ---- list_model_fetch.cpp ----

        bool canFetchMore() const override;

        // Порция подгрузчика встаёт в конец; повторный вход из уведомления игнорируется / The fetcher portion goes to the end; a reentry from a notification is ignored
        void fetchMore() override;

    private:
        std::vector<Record> _records;
        ColumnSet _columns;
        std::shared_ptr<ListFetcher> _fetcher;
        bool _fetching = false;
    };
}

#endif
