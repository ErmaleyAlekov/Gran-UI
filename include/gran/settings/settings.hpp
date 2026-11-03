#ifndef GRAN_SETTINGS_SETTINGS_HPP
#define GRAN_SETTINGS_SETTINGS_HPP

#include "gran/platform/window.hpp"

#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace gran::settings
{
    // Тип значения настройки (атрибут type в XML) / A setting value type (the type attribute in XML)
    enum class ValueType : std::uint8_t { STRING, NUMBER, BOOLEAN, LIST };

    // Значение: текст (число и логическое — тоже текстом, как в XML) или список строк / A value: a text (a number and a boolean — as text too, as in XML) or a list of strings
    struct Value
    {
        ValueType type = ValueType::STRING;
        std::string text;
        std::vector<std::string> items;
    };

    // ---- settings_file.cpp ----

    // Файл настроек по умолчанию: %APPDATA%\<программа>\settings.xml или $XDG_CONFIG_HOME/<программа>/settings.xml (UTF-8)
    // The default settings file: %APPDATA%\<program>\settings.xml or $XDG_CONFIG_HOME/<program>/settings.xml (UTF-8)
    std::string defaultPath(const std::string& appName);

    // Ключ сохранённого атрибута узла: окно (layout), id узла, имя атрибута / The key of a saved node attribute: the window (layout), the node id, the attribute name
    using AttributeKey = std::tuple<std::string, std::string, std::string>;

    /*
    ** Настройки программы в XML со своей схемой (schemas/gran-settings.xsd): значения программы, раскладка
    ** окон (место, размер, развёрнутость) и сохранённые атрибуты узлов (persist), переменные выражений
    ** с persist. Паттерн Memento: состояние интерфейса снимается в настройки и восстанавливается из них.
    ** Только поток интерфейса.
    **
    ** The program settings in XML with its own schema (schemas/gran-settings.xsd): the program values, the window layout
    ** (the place, the size, the maximization) and the saved node attributes (persist), the expression variables
    ** with persist. The Memento pattern: the UI state is taken into the settings and restored from them.
    ** The UI thread only.
    */
    class Settings
    {
    public:
        // ---- settings_file.cpp ----

        /*
        ** Загрузить файл (UTF-8 путь). Нет файла — пустые настройки, true. Испорченный файл (XML или схема) —
        ** переименовывается в .bad, предупреждение в журнал, пустые настройки, false.
        **
        ** Load the file (a UTF-8 path). No file — empty settings, true. A damaged file (XML or the schema) —
        ** renamed into .bad, a warning to the log, empty settings, false.
        */
        bool load(const std::string& path);

        // Сохранить атомарно (временный файл + переименование); каталог создаётся; false — не записано / Save atomically (a temporary file + renaming); the directory is created; false — not written
        bool save();

        // Путь файла (UTF-8) и признак несохранённых изменений / The file path (UTF-8) and the unsaved changes flag
        const std::string& path() const noexcept;
        bool modified() const noexcept;

        // ---- settings_values.cpp ----

        // Значение по имени или fallback (нет или другого типа) / A value by name or fallback (missing or of another type)
        std::string getString(const std::string& name, const std::string& fallback = std::string()) const;
        double getNumber(const std::string& name, double fallback = 0.0) const;
        bool getBool(const std::string& name, bool fallback = false) const;
        std::vector<std::string> getList(const std::string& name) const;

        // Значение с типом (nullptr — нет): для обёрток языков / The value with its type (nullptr — none): for the language wrappers
        const Value* find(const std::string& name) const;

        // ---- settings_edit.cpp ----
        void setString(const std::string& name, const std::string& value);
        void setNumber(const std::string& name, double value);
        void setBool(const std::string& name, bool value);
        void setList(const std::string& name, const std::vector<std::string>& items);
        bool remove(const std::string& name);

        // ---- settings_layout.cpp: раскладка окон и узлов (их пишет и читает движок) ----
        // ---- settings_layout.cpp: the window and node layout (the engine writes and reads it) ----
        bool windowPlacement(const std::string& layout, platform::WindowPlacement& placement) const;
        void setWindowPlacement(const std::string& layout, const platform::WindowPlacement& placement);
        bool attribute(const AttributeKey& key, std::string& value) const;
        void setAttribute(const AttributeKey& key, const std::string& value);

        // ---- settings_variables.cpp: переменные выражений с persist="true" ----
        // ---- settings_variables.cpp: the expression variables with persist="true" ----
        bool variable(const std::string& name, Value& value) const;
        void setVariable(const std::string& name, const Value& value);

        // Модель целиком — для разбора и записи XML (settings_read.cpp, settings_write.cpp) / The whole model — for parsing and writing XML
        struct Model
        {
            std::map<std::string, Value> values;
            std::map<std::string, Value> variables;
            std::map<std::string, platform::WindowPlacement> windows;
            std::map<AttributeKey, std::string> attributes;
        };

    private:
        Model _model;
        std::string _path;
        bool _modified = false;
    };
}

#endif
