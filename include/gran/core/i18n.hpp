#ifndef GRAN_CORE_I18N_HPP
#define GRAN_CORE_I18N_HPP

#include "gran/core/message.hpp"

#include <cstddef>
#include <initializer_list>
#include <string>
#include <string_view>

namespace gran::i18n
{
    /*
    ** Запись каталога: идентификатор и шаблон текста с метками {0}, {1}...
    ** Идентификатор хранится явно: порядок строк в каталоге не важен.
    **
    ** A catalog entry: an identifier and a text template with {0}, {1}... placeholders.
    ** The identifier is stored explicitly: the row order in a catalog does not matter.
    */
    struct MessageText
    {
        Message id;
        const char* text;
    };

    /*
    ** Каталог языка (паттерн Strategy: язык — сменяемая стратегия получения текста).
    **
    ** A language catalog (the Strategy pattern: a language is an interchangeable text source).
    */
    struct Catalog
    {
        const char* code;
        const MessageText* entries;
        std::size_t size;
    };

    // Каталоги языков из src/core/i18n/catalogs/
    // The language catalogs from src/core/i18n/catalogs/
    extern const Catalog RUSSIAN_CATALOG;
    extern const Catalog ENGLISH_CATALOG;

    /*
    ** Текст сообщения на текущем языке с подставленными аргументами;
    ** без перевода — английский текст.
    **
    ** The message text in the current language with the arguments substituted;
    ** without a translation — the English text.
    */
    std::string format(Message id, std::initializer_list<std::string_view> arguments = {});

    // Смена языка для всех потоков; false — языка нет, язык прежний
    // Changes the language for all threads; false — no such language, the language stays
    bool setLanguage(std::string_view code) noexcept;

    // Код текущего языка ("ru", "en")
    // The current language code ("ru", "en")
    const char* language() noexcept;

    // Шаблон сообщения в каталоге или nullptr / The message template in a catalog or nullptr
    const char* findTemplate(const Catalog& catalog, Message id) noexcept;

    // Текущий каталог (для format) / The current catalog (for format)
    const Catalog& currentCatalog() noexcept;
}

#endif
