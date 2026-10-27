#ifndef GRAN_DATA_LIST_MODEL_HPP
#define GRAN_DATA_LIST_MODEL_HPP

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace gran::data
{
    // Запись списка: имя поля → текст (как атрибуты элемента <Item>)
    // A list record: a field name → a text (like the attributes of an <Item> element)
    using Record = std::map<std::string, std::string, std::less<>>;

    /*
    ** Наблюдатель модели (паттерн Observer). Привязка списка в разметке подписывается на модель
    ** и по каждому уведомлению правит только затронутые узлы, не перестраивая весь список.
    **
    ** A model observer (the Observer pattern). A list binding in the markup subscribes to the model
    ** and on each notification edits only the affected nodes without rebuilding the whole list.
    */
    class ListObserver
    {
    public:
        virtual ~ListObserver() = default;

        // Вставлено count записей начиная с index / count records were inserted starting at index
        virtual void inserted(std::size_t index, std::size_t count) = 0;

        // Удалено count записей начиная с index / count records were removed starting at index
        virtual void removed(std::size_t index, std::size_t count) = 0;

        // Поля записи index изменились / The fields of the record index changed
        virtual void changed(std::size_t index) = 0;
    };

    /*
    ** Модель списка: упорядоченные записи и подписчики. Данные живут в ядре, а не в коде
    ** приложения: разметка описывает вид одной записи (<ItemTemplate>), модель — сами записи.
    ** Работает только в потоке интерфейса; фоновые потоки правят её через Application::post.
    **
    ** A list model: ordered records and subscribers. The data lives in the core, not in the application
    ** code: the markup describes the look of one record (<ItemTemplate>), the model — the records themselves.
    ** It works only in the UI thread; background threads edit it through Application::post.
    */
    class ListModel
    {
    public:
        // ---- list_model.cpp ----

        // Число записей / The record count
        std::size_t size() const noexcept;

        // Запись по номеру; вне диапазона — GranException(INVALID_ARGUMENT) / A record by index; out of range — GranException(INVALID_ARGUMENT)
        const Record& at(std::size_t index) const;

        // Вставить запись перед index (index == size() — в конец) / Insert a record before index (index == size() — at the end)
        void insert(std::size_t index, Record record);

        // Добавить запись в конец / Append a record at the end
        void append(Record record);

        // Удалить запись / Remove a record
        void remove(std::size_t index);

        // ---- list_model_observers.cpp ----

        // Сменить одно поле записи (подписчики получают changed) / Change one record field (the subscribers get changed)
        void set(std::size_t index, const std::string& field, std::string value);

        // Удалить все записи / Remove all records
        void clear();

        // Подписать наблюдателя (без владения; повторная подписка игнорируется) / Subscribe an observer (not owned; a repeated subscription is ignored)
        void subscribe(ListObserver* observer);

        // Отписать наблюдателя (можно и во время уведомления) / Unsubscribe an observer (allowed during a notification too)
        void unsubscribe(ListObserver* observer) noexcept;

        // Подписан ли наблюдатель / Whether the observer is subscribed
        bool subscribed(const ListObserver* observer) const noexcept;

    private:
        std::vector<Record> _records;
        std::vector<ListObserver*> _observers;
    };
}

#endif
