#include "gran/core/i18n.hpp"

#include <atomic>

/*
** Язык по умолчанию задаётся при сборке: -DGRAN_LANG=en. Без него — русский.
**
** The default language is chosen at build time: -DGRAN_LANG=en. Without it — Russian.
*/
#ifndef GRAN_LANG
    #define GRAN_LANG ru
#endif
#define GRAN_STRINGIZE_TOKEN(token) #token
#define GRAN_STRINGIZE(token) GRAN_STRINGIZE_TOKEN(token)

namespace gran::i18n
{
    namespace
    {
        // Все каталоги; первый подходящий по коду становится текущим
        // All catalogs; the first one matching the code becomes current
        const Catalog* const CATALOGS[] = {&RUSSIAN_CATALOG, &ENGLISH_CATALOG};

        // Ищет каталог по коду / Finds a catalog by its code
        const Catalog* findCatalog(std::string_view code) noexcept
        {
            for (const Catalog* catalog : CATALOGS)
            {
                if (code == catalog->code)
                    return catalog;
            }
            return nullptr;
        }

        // Текущий каталог: атомарный указатель, смена видна всем потокам
        // The current catalog: an atomic pointer, a change is visible to all threads
        std::atomic<const Catalog*>& currentSlot() noexcept
        {
            static std::atomic<const Catalog*> slot{nullptr};
            return slot;
        }
    }

    // Смена языка; неизвестный код отвергается
    // Changes the language; an unknown code is rejected
    bool setLanguage(std::string_view code) noexcept
    {
        const Catalog* catalog = findCatalog(code);

        if (catalog == nullptr)
            return false;
        currentSlot().store(catalog);
        return true;
    }

    // Код текущего языка / The current language code
    const char* language() noexcept
    {
        return currentCatalog().code;
    }

    // Текущий каталог; при первом обращении — язык сборки (или английский)
    // The current catalog; on first access — the build language (or English)
    const Catalog& currentCatalog() noexcept
    {
        const Catalog* catalog = currentSlot().load();

        if (catalog != nullptr)
            return *catalog;
        catalog = findCatalog(GRAN_STRINGIZE(GRAN_LANG));
        if (catalog == nullptr)
            catalog = &ENGLISH_CATALOG;
        currentSlot().store(catalog);
        return *catalog;
    }
}
