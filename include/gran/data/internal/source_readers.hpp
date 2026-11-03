#ifndef GRAN_DATA_INTERNAL_SOURCE_READERS_HPP
#define GRAN_DATA_INTERNAL_SOURCE_READERS_HPP

#include "gran/core/gran_exception.hpp"
#include "gran/data/data_source.hpp"
#include "gran/data/json_reader.hpp"

#include "sax/content_handler.hpp"

#include <condition_variable>
#include <istream>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

namespace beresta
{
    class Document;
    class Element;
    namespace xpath
    {
        class XPathExpression;
    }
}

namespace gran::data::internal
{
    // ---- json_source_select.cpp ----

    // Перейти по пути select к массиву записей (читатель встаёт после его «[») / Go along the select path to the record array (the reader stops after its "[")
    void selectJsonArray(JsonReader& reader, const std::string& select);

    // ---- json_source_record.cpp ----

    // Прочитать элемент массива, начатый лексемой first, в узел (запись и дети) / Read an array element started by the first token into a node (the record and the children)
    void readJsonItem(JsonReader& reader, JsonToken first, const SourceSpec& spec, TreeItem& item);

    // Поля по описанию Field (без описания — запись как есть) / The fields by the Field description (without a description — the record as it is)
    Record mapFields(const SourceSpec& spec, Record record);

    /*
    ** Подгрузчик списка из JSON: файл открыт всё время подгрузки, читатель стоит внутри массива
    ** записей и продолжает с того же места.
    **
    ** A list fetcher from JSON: the file is open during the whole loading, the reader stands inside the record
    ** array and continues from the same place.
    */
    class JsonListFetcher final : public ListFetcher
    {
    public:
        // ---- json_source_fetcher.cpp ----
        explicit JsonListFetcher(SourceSpec spec);
        bool hasMore() const override;
        std::vector<Record> next() override;

    private:
        SourceSpec _spec;
        // Файл диска или ресурс res:/ (поток из памяти); объявлен до _reader — читатель ссылается на него
        // A disk file or a res:/ resource (a stream from memory); declared before _reader — the reader refers to it
        std::unique_ptr<std::istream> _file;
        JsonReader _reader;
        bool _done = false;
    };

    // Дерево из JSON (json_source_fetcher.cpp) / A tree from JSON (json_source_fetcher.cpp)
    std::vector<TreeItem> readJsonTree(const SourceSpec& spec);

    // Получатель записей XML (паттерн Observer для обработчика SAX) / The XML record receiver (the Observer pattern for the SAX handler)
    class XmlRecordSink
    {
    public:
        virtual ~XmlRecordSink() = default;

        // Готов узел записи (с детьми для дерева) / A record node is ready (with children for a tree)
        virtual void record(TreeItem item) = 0;
    };

    // Шаг пути select для XML: имя (или *) и «//» перед ним / A select path step for XML: a name (or *) and "//" before it
    struct XmlStep
    {
        std::string name;
        bool descendant = false;
    };

    // Поля записи XML: имя и скомпилированное выражение / The XML record fields: the name and the compiled expression
    using FieldPaths = std::vector<std::pair<std::string, std::shared_ptr<const beresta::xpath::XPathExpression>>>;

    // ---- xml_source_path.cpp ----

    // Шаги пути; пусто — дети корня (/*/*); ошибка — GranException(INVALID_ARGUMENT) / The path steps; empty — the root children (/*/*); an error is GranException(INVALID_ARGUMENT)
    std::vector<XmlStep> parseXmlPath(const std::string& select);

    // Подходит ли путь открытых элементов (от корня документа) под шаги / Whether the path of the open elements (from the document root) fits the steps
    bool matchXmlPath(const std::vector<XmlStep>& steps, const std::vector<std::string>& stack);

    // ---- xml_source_convert.cpp ----

    // Узел из элемента записи: поля и дети с именем spec.children / A node from a record element: the fields and the children named spec.children
    TreeItem convertElement(const beresta::Element& element, const SourceSpec& spec, const FieldPaths& fields);

    // Скомпилировать пути полей; ошибка — GranException(INVALID_ARGUMENT) с путём / Compile the field paths; an error is GranException(INVALID_ARGUMENT) with the path
    FieldPaths compileFields(const SourceSpec& spec);

    /*
    ** Обработчик SAX: следит за путём открытых элементов и, найдя запись, собирает из её событий
    ** маленький DOM «Берёсты» (только эта запись), по которому затем вычисляются поля XPath.
    **
    ** A SAX handler: it tracks the path of the open elements and, having found a record, builds from its events
    ** a small Beresta DOM (only this record), over which the XPath fields are evaluated then.
    */
    class XmlRecordHandler final : public beresta::ContentHandler
    {
    public:
        // ---- xml_source_handler.cpp ----
        XmlRecordHandler(const SourceSpec& spec, const FieldPaths& fields, XmlRecordSink& sink);
        ~XmlRecordHandler() override;
        void startElement(const beresta::SaxElement& element) override;
        void endElement(const beresta::SaxElement& element) override;
        void characters(std::string_view text) override;
        void cdata(std::string_view text) override;

    private:
        const SourceSpec& _spec;
        const FieldPaths& _fields;
        XmlRecordSink& _sink;
        std::vector<XmlStep> _steps;
        std::vector<std::string> _stack;
        std::unique_ptr<beresta::Document> _document;
        std::vector<beresta::Element*> _open;
    };

    // ---- xml_source_parse.cpp ----

    // Разобрать файл SAX «Берёсты», отдавая записи получателю; ошибки — DataException / Parse the file with Beresta SAX handing the records to the receiver; errors are DataException
    void parseXmlSource(const SourceSpec& spec, XmlRecordSink& sink);

    /*
    ** Подгрузчик списка из XML. Без порции — весь файл за первый next() в вызывающем потоке.
    ** С порцией SAX «Берёсты» (только проталкивающий) работает в рабочем потоке: после каждых
    ** fetch записей он ждёт, пока поток интерфейса заберёт порцию (передача из рук в руки), поэтому
    ** в памяти не больше одной порции, а результат не зависит от планировщика. Деструктор будит
    ** поток с признаком отмены и дожидается его.
    **
    ** A list fetcher from XML. Without a portion — the whole file in the first next() in the calling thread.
    ** With a portion Beresta SAX (push only) works in a worker thread: after every fetch records it waits
    ** until the UI thread takes the portion (a hand-over), so no more than one portion is in memory, and the result
    ** does not depend on the scheduler. The destructor wakes the thread with a cancel flag and waits for it.
    */
    class XmlListFetcher final : public ListFetcher, private XmlRecordSink
    {
    public:
        // ---- xml_source_fetcher.cpp ----
        explicit XmlListFetcher(SourceSpec spec);
        ~XmlListFetcher() override;
        XmlListFetcher(const XmlListFetcher&) = delete;
        XmlListFetcher& operator=(const XmlListFetcher&) = delete;
        bool hasMore() const override;
        std::vector<Record> next() override;

    private:
        // ---- xml_source_worker.cpp ----

        // Запись от обработчика SAX (в рабочем потоке ждёт, пока заберут порцию) / A record from the SAX handler (in the worker thread it waits until the portion is taken)
        void record(TreeItem item) override;

        // Тело рабочего потока / The worker thread body
        void work();

        // Дождаться порции или конца разбора и забрать её (под замком) / Wait for a portion or the parsing end and take it (under the lock)
        std::vector<Record> takePortion(std::unique_lock<std::mutex>& lock);

        SourceSpec _spec;
        mutable std::mutex _mutex;
        std::condition_variable _ready;
        std::condition_variable _taken;
        std::vector<Record> _queue;
        // Ошибка разбора для потока интерфейса (текст, место) / A parsing error for the UI thread (the text, the place)
        std::unique_ptr<DataException> _error;
        bool _finished = false;
        bool _cancelled = false;
        bool _started = false;
        // Поток последним: он уничтожается (после join) раньше полей, которые читает / The thread is last: it is destroyed (after join) before the fields it reads
        std::thread _worker;
    };

    // Дерево из XML (xml_source_parse.cpp) / A tree from XML (xml_source_parse.cpp)
    std::vector<TreeItem> readXmlTree(const SourceSpec& spec);
}

#endif
