#include "gran/core/i18n.hpp"

namespace gran::i18n
{
    namespace
    {
        // Русские тексты сообщений; {N} — аргументы format()
        // Russian message texts; {N} are the format() arguments
        const MessageText TEXTS[] = {
            {Message::CAPI_ARGUMENT, "недопустимый аргумент или размер структуры ABI"},
            {Message::CAPI_NODE, "узел с id \"{0}\" не найден"},
            {Message::CAPI_PROPERTY, "неподдерживаемое свойство узла \"{0}\""},
            {Message::CAPI_COMMAND, "неподдерживаемая команда приложения \"{0}\""},
            {Message::CAPI_THREAD, "операция должна выполняться в создавшем приложение потоке UI"},
            {Message::CAPI_BUSY, "нельзя уничтожать приложение или повторно входить в run во время его работы"},
            {Message::CAPI_UNKNOWN, "непредвиденная ошибка нативного кода"},
            {Message::STYLE_BAD_NUMBER, "ожидалось число, найдено \"{0}\""},
            {Message::STYLE_BAD_COLOR, "неверный цвет \"{0}\": ожидалось #rgb, #rrggbb, #rrggbbaa, rgb(), hsl(), имя цвета или $имя"},
            {Message::STYLE_BAD_LENGTH, "неверная длина \"{0}\": ожидалось число пикселей, проценты или auto"},
            {Message::STYLE_BAD_LIST_SIZE, "в значении \"{0}\" должно быть {1}"},
            {Message::STYLE_BAD_KEYWORD, "неизвестное значение \"{0}\", допустимо: {1}"},
            {Message::STYLE_BAD_SHADOW, "неверная тень \"{0}\": ожидалось «x y [размытие [растяжение]] цвет [inset]»"},
            {Message::STYLE_BAD_GRADIENT, "неверный градиент \"{0}\": {1}"},
            {Message::STYLE_BAD_EASING, "неверная кривая анимации \"{0}\""},
            {Message::STYLE_BAD_DURATION, "неверная длительность \"{0}\": ожидалось 200ms или 0.2s"},
            {Message::STYLE_BAD_TRANSITION, "неверный переход \"{0}\": ожидалось «свойство длительность [кривая] [задержка]»"},
            {Message::STYLE_BAD_ANIMATION, "неверная анимация \"{0}\": ожидалось «имя длительность [кривая] [задержка] [повторы|infinite] [направление] [заполнение]»"},
            {Message::STYLE_BAD_TRANSFORM, "неверное преобразование \"{0}\""},
            {Message::STYLE_BAD_TRACK, "неверная дорожка сетки \"{0}\": ожидалось число, auto, Nfr или minmax(a, b)"},
            {Message::STYLE_UNKNOWN_PROPERTY, "неизвестное свойство \"{0}\""},
            {Message::MARKUP_AT_POSITION, "строка {0}, столбец {1}: {2}"},
            {Message::MARKUP_ATTRIBUTE_VALUE, "<{0} {1}=\"…\">: {2}"},
            {Message::MARKUP_ROOT_NOT_WINDOW, "корневой элемент разметки должен быть <Window>, найден <{0}>"},
            {Message::MARKUP_UNKNOWN_ELEMENT, "неизвестный элемент <{0}>"},
            {Message::MARKUP_TOO_MANY_NODES, "в разметке больше {0} элементов"},
            {Message::RESOURCE_UNKNOWN_STYLE, "стиль \"{0}\" не объявлен"},
            {Message::RESOURCE_UNKNOWN_TOKEN, "значение темы \"${0}\" не объявлено"},
            {Message::RESOURCE_STYLE_CYCLE, "стиль \"{0}\" наследует сам себя через цепочку basedOn"},
            {Message::RESOURCE_UNKNOWN_ACTION, "действие \"{0}\" не зарегистрировано"},
            {Message::RESOURCE_UNKNOWN_KEYFRAMES, "ключевые кадры \"{0}\" не объявлены (<Keyframes name=\"{0}\">)"},
            {Message::FONT_BAD_FILE, "файл шрифта \"{0}\" повреждён: {1}"},
            {Message::FONT_MISSING_TABLE, "в шрифте нет обязательной таблицы \"{0}\""},
            {Message::FONT_NOT_FOUND, "шрифт \"{0}\" не найден"},
            {Message::IMAGE_BAD_FILE, "изображение \"{0}\" повреждено или не поддерживается: {1}"},
            {Message::IO_READ_FAILED, "не удалось прочитать файл \"{0}\""},
            {Message::IO_WRITE_FAILED, "не удалось записать файл \"{0}\""},
            {Message::GPU_CALL_FAILED, "{0} вернул ошибку {1} ({2}:{3})"},
            {Message::GPU_DEVICE_LOST, "устройство GPU потеряно: {0}"},
            {Message::GPU_NO_ADAPTER, "не найден видеоадаптер с поддержкой {0}"},
            {Message::GPU_BUFFER_OVERFLOW, "буфер GPU \"{0}\" переполнен: нужно {1} байт, ёмкость {2}"},
            {Message::GPU_DESCRIPTOR_EXHAUSTED, "закончились дескрипторы текстур (предел {0})"},
            {Message::GPU_BAD_TEXTURE, "неверная операция с текстурой в слоте {0}: {1}"},
            {Message::GPU_DEBUG_MESSAGE, "отладочный слой {0}: {1}"},
            {Message::APP_NO_MARKUP, "разметка не загружена: вызовите loadFile или loadString до run"},
            {Message::APP_REMOVE_ROOT, "корневой элемент нельзя удалить: у него нет родителя"},
            {Message::MENU_COPY, "Копировать"},
            {Message::MENU_PASTE, "Вставить"},
            {Message::MENU_SELECT_ALL, "Выделить всё"},
            {Message::DATA_INDEX_OUT_OF_RANGE, "запись {0} вне списка из {1} записей"},
            {Message::MARKUP_FRAGMENT_PARENT, "фрагмент разметки нельзя вставить в {0}: у этого элемента нет детей-элементов"},
            {Message::WEBVIEW_WRONG_NODE, "элемент {0} — не WebView"},
            {Message::WEBVIEW_UNAVAILABLE, "встроенный браузер недоступен (код 0x{0}): нужен WebView2Loader.dll рядом с программой и среда WebView2 Runtime"},
            {Message::APP_WRONG_ELEMENT, "элемент {0} здесь не подходит: нужен {1}"},
            {Message::PLATFORM_WINDOW_FAILED, "не удалось создать окно: {0}"},
            {Message::PLATFORM_FILE_DIALOG_FAILED, "системный диалог файлов не открылся (код 0x{0})"},
            {Message::ICON_BAD_PATH, "данные пути иконки неверны в позиции {0}"},
            {Message::ICON_BAD_VIEWBOX, "viewBox иконки должен быть «x y ширина высота» с положительными размерами: \"{0}\""},
            {Message::ICON_BAD_SVG, "файл иконки \"{0}\" не подходит: {1}"},
            {Message::APP_RELOAD_NO_FILE, "перезагрузка требует документа, загруженного из файла"},
            {Message::WINDOW_MINIMIZE, "Свернуть"},
            {Message::WINDOW_MAXIMIZE, "Развернуть"},
            {Message::WINDOW_RESTORE, "Восстановить"},
            {Message::WINDOW_CLOSE, "Закрыть"},
            {Message::PLATFORM_FILE_DIALOG_TOOL, "системный диалог файлов недоступен: нужна программа zenity или kdialog"}
        };
    }

    // Каталог русского языка / The Russian language catalog
    const Catalog RUSSIAN_CATALOG{"ru", TEXTS, sizeof(TEXTS) / sizeof(TEXTS[0])};
}
