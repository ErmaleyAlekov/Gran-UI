#ifndef GRAN_CORE_ERROR_CODE_HPP
#define GRAN_CORE_ERROR_CODE_HPP

namespace gran
{
    /*
    ** Категории ошибок движка. Код позволяет программно различать сбои,
    ** не разбирая текст сообщения (текст зависит от выбранного языка).
    **
    ** The engine error categories. The code allows telling failures apart
    ** programmatically without parsing the message text (the text depends on the language).
    */
    enum class ErrorCode
    {
        MARKUP,            // Разметка не прошла разбор или проверку схемы
                           // The markup failed parsing or the schema check
        STYLE_VALUE,       // Значение свойства стиля записано неверно
                           // A style property value is written incorrectly
        RESOURCE,          // Ссылка на несуществующий стиль, цвет темы, шрифт, картинку
                           // A reference to a missing style, theme color, font, image
        FONT,              // Файл шрифта повреждён или не поддерживается
                           // A font file is damaged or not supported
        IMAGE,             // Файл изображения повреждён или не поддерживается
                           // An image file is damaged or not supported
        GPU,               // Вызов графического API вернул ошибку
                           // A graphics API call returned an error
        DEVICE_LOST,       // Устройство GPU потеряно (сброс драйвера); нужно пересоздание
                           // The GPU device is lost (a driver reset); it must be recreated
        PLATFORM,          // Ошибка оконной системы ОС / An OS windowing system error
        IO,                // Ошибка чтения или записи файла / A file read or write error
        INVALID_ARGUMENT,  // Недопустимый аргумент вызова API / An invalid API call argument
        LIMIT_EXCEEDED     // Превышен защитный предел (размер, глубина, число элементов)
                           // A protective limit is exceeded (size, depth, element count)
    };

    // Количество кодов ошибок (размер таблицы имён)
    // The number of error codes (the size of the name table)
    constexpr int ERROR_CODE_COUNT = 11;

    // Имя кода ("MARKUP", "GPU"...) — одинаково на всех языках
    // The code name ("MARKUP", "GPU"...) — the same in all languages
    const char* errorCodeName(ErrorCode code) noexcept;
}

#endif
