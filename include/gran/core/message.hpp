#ifndef GRAN_CORE_MESSAGE_HPP
#define GRAN_CORE_MESSAGE_HPP

#include <cstdint>

namespace gran::i18n
{
    /*
    ** Идентификаторы всех сообщений движка. Тексты на каждом языке лежат в каталогах
    ** src/core/i18n/catalogs/messages_<код>.cpp; код ссылается только на идентификатор.
    ** Новое сообщение добавляется перед COUNT и во все каталоги; без перевода берётся английский.
    **
    ** Identifiers of all engine messages. The texts in each language live in the catalogs
    ** src/core/i18n/catalogs/messages_<code>.cpp; code refers only to the identifier.
    ** A new message is added before COUNT and to all catalogs; without a translation English is used.
    */
    enum class Message : std::uint16_t
    {
        // ---- Значения стилей / Style values ----
        STYLE_BAD_NUMBER,
        STYLE_BAD_COLOR,
        STYLE_BAD_LENGTH,
        STYLE_BAD_LIST_SIZE,
        STYLE_BAD_KEYWORD,
        STYLE_BAD_SHADOW,
        STYLE_BAD_GRADIENT,
        STYLE_BAD_EASING,
        STYLE_BAD_DURATION,
        STYLE_BAD_TRANSITION,
        STYLE_BAD_ANIMATION,
        STYLE_BAD_TRANSFORM,
        STYLE_BAD_TRACK,
        STYLE_UNKNOWN_PROPERTY,

        // ---- Разметка и ресурсы / Markup and resources ----
        MARKUP_AT_POSITION,
        MARKUP_ATTRIBUTE_VALUE,
        MARKUP_ROOT_NOT_WINDOW,
        MARKUP_UNKNOWN_ELEMENT,
        MARKUP_TOO_MANY_NODES,
        RESOURCE_UNKNOWN_STYLE,
        RESOURCE_UNKNOWN_TOKEN,
        RESOURCE_STYLE_CYCLE,
        RESOURCE_UNKNOWN_ACTION,
        RESOURCE_UNKNOWN_KEYFRAMES,

        // ---- Шрифты и изображения / Fonts and images ----
        FONT_BAD_FILE,
        FONT_MISSING_TABLE,
        FONT_NOT_FOUND,
        IMAGE_BAD_FILE,
        IO_READ_FAILED,
        IO_WRITE_FAILED,

        // ---- GPU и окно / GPU and window ----
        GPU_CALL_FAILED,
        GPU_DEVICE_LOST,
        GPU_NO_ADAPTER,
        GPU_BUFFER_OVERFLOW,
        GPU_DESCRIPTOR_EXHAUSTED,
        GPU_BAD_TEXTURE,
        GPU_DEBUG_MESSAGE,
        PLATFORM_WINDOW_FAILED,
        APP_NO_MARKUP,
        APP_REMOVE_ROOT,
        MENU_COPY,
        MENU_PASTE,
        MENU_SELECT_ALL,
        DATA_INDEX_OUT_OF_RANGE,
        MARKUP_FRAGMENT_PARENT,
        WEBVIEW_WRONG_NODE,
        WEBVIEW_UNAVAILABLE,
        WEBVIEW_NOT_BUILT,
        APP_WRONG_ELEMENT,
        PLATFORM_FILE_DIALOG_FAILED,
        ICON_BAD_PATH,
        ICON_BAD_VIEWBOX,
        ICON_BAD_SVG,

        APP_RELOAD_NO_FILE,
        WINDOW_MINIMIZE,
        WINDOW_MAXIMIZE,
        WINDOW_RESTORE,
        WINDOW_CLOSE,
        CAPI_ARGUMENT,
        CAPI_NODE,
        CAPI_PROPERTY,
        CAPI_COMMAND,
        CAPI_THREAD,
        CAPI_BUSY,
        CAPI_UNKNOWN,
        PLATFORM_FILE_DIALOG_TOOL,
        GPU_VULKAN_LOADER,
        MARKUP_BAD_SHORTCUT,
        APP_UNKNOWN_COMMAND,
        MENU_UNDO,
        MENU_REDO,
        MARKUP_NOT_LIVE,
        MARKUP_BAD_ATTRIBUTE,
        COUNT
    };
}

#endif
