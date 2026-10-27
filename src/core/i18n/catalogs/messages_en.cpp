#include "gran/core/i18n.hpp"

namespace gran::i18n
{
    namespace
    {
        // Английские тексты сообщений; {N} — аргументы format()
        // English message texts; {N} are the format() arguments
        const MessageText TEXTS[] = {
            {Message::STYLE_BAD_NUMBER, "expected a number, found \"{0}\""},
            {Message::STYLE_BAD_COLOR, "invalid color \"{0}\": expected #rgb, #rrggbb, #rrggbbaa, rgb(), hsl(), color name, or $name"},
            {Message::STYLE_BAD_LENGTH, "invalid length \"{0}\": expected pixels, percentages, or auto"},
            {Message::STYLE_BAD_LIST_SIZE, "value \"{0}\" must have {1}"},
            {Message::STYLE_BAD_KEYWORD, "unknown value \"{0}\", allowed: {1}"},
            {Message::STYLE_BAD_SHADOW, "invalid shadow \"{0}\": expected «x y [blur [spread]] color [inset]»"},
            {Message::STYLE_BAD_GRADIENT, "invalid gradient \"{0}\": {1}"},
            {Message::STYLE_BAD_EASING, "invalid animation curve \"{0}\""},
            {Message::STYLE_BAD_DURATION, "invalid duration \"{0}\": expected 200ms or 0.2s"},
            {Message::STYLE_BAD_TRANSITION, "invalid transition \"{0}\": expected «property duration [curve] [delay]»"},
            {Message::STYLE_BAD_TRANSFORM, "invalid transform \"{0}\""},
            {Message::STYLE_BAD_TRACK, "invalid grid track \"{0}\": expected number, auto, Nfr, or minmax(a, b)"},
            {Message::STYLE_UNKNOWN_PROPERTY, "unknown property \"{0}\""},
            {Message::MARKUP_AT_POSITION, "line {0}, column {1}: {2}"},
            {Message::MARKUP_ATTRIBUTE_VALUE, "<{0} {1}=\"…\">: {2}"},
            {Message::MARKUP_ROOT_NOT_WINDOW, "root element of markup must be <Window>, found <{0}>"},
            {Message::MARKUP_UNKNOWN_ELEMENT, "unknown element <{0}>"},
            {Message::MARKUP_TOO_MANY_NODES, "markup has more than {0} elements"},
            {Message::RESOURCE_UNKNOWN_STYLE, "style \"{0}\" is not declared"},
            {Message::RESOURCE_UNKNOWN_TOKEN, "theme value \"${0}\" is not declared"},
            {Message::RESOURCE_STYLE_CYCLE, "style \"{0}\" inherits itself through the basedOn chain"},
            {Message::RESOURCE_UNKNOWN_ACTION, "action \"{0}\" is not registered"},
            {Message::FONT_BAD_FILE, "font file \"{0}\" is corrupted: {1}"},
            {Message::FONT_MISSING_TABLE, "font is missing required table \"{0}\""},
            {Message::FONT_NOT_FOUND, "font \"{0}\" not found"},
            {Message::IMAGE_BAD_FILE, "image \"{0}\" is corrupted or not supported: {1}"},
            {Message::IO_READ_FAILED, "failed to read file \"{0}\""},
            {Message::IO_WRITE_FAILED, "failed to write file \"{0}\""},
            {Message::GPU_CALL_FAILED, "{0} returned error {1} ({2}:{3})"},
            {Message::GPU_DEVICE_LOST, "GPU device lost: {0}"},
            {Message::GPU_NO_ADAPTER, "no video adapter found supporting {0}"},
            {Message::GPU_BUFFER_OVERFLOW, "GPU buffer \"{0}\" overflowed: need {1} bytes, capacity {2}"},
            {Message::GPU_DESCRIPTOR_EXHAUSTED, "texture descriptors exhausted (limit {0})"},
            {Message::GPU_BAD_TEXTURE, "invalid texture operation in slot {0}: {1}"},
            {Message::GPU_DEBUG_MESSAGE, "debug layer {0}: {1}"},
            {Message::APP_NO_MARKUP, "no markup is loaded: call loadFile or loadString before run"},
            {Message::APP_REMOVE_ROOT, "the root element cannot be removed: it has no parent"},
            {Message::MENU_COPY, "Copy"},
            {Message::MENU_PASTE, "Paste"},
            {Message::MENU_SELECT_ALL, "Select all"},
            {Message::DATA_INDEX_OUT_OF_RANGE, "record {0} is outside a list of {1} records"},
            {Message::MARKUP_FRAGMENT_PARENT, "a markup fragment cannot be inserted into {0}: this element has no element children"},
            {Message::WEBVIEW_WRONG_NODE, "element {0} is not a WebView"},
            {Message::WEBVIEW_UNAVAILABLE, "the embedded browser is unavailable (code 0x{0}): WebView2Loader.dll next to the program and the WebView2 Runtime are needed"},
            {Message::APP_WRONG_ELEMENT, "element {0} does not fit here: {1} is needed"},
            {Message::PLATFORM_WINDOW_FAILED, "failed to create window: {0}"}
        };
    }

    // Каталог английского языка / The English language catalog
    const Catalog ENGLISH_CATALOG{"en", TEXTS, sizeof(TEXTS) / sizeof(TEXTS[0])};
}
