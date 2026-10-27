#ifndef GRAN_TEST_SUITES_HPP
#define GRAN_TEST_SUITES_HPP

#include "framework/test_framework.hpp"

namespace suites
{
    // ---- Ядро и утилиты / Core and utilities ----
    void registerI18nTests(test::TestRunner& runner);
    void registerGeometryTests(test::TestRunner& runner);

    // ---- Значения стилей / Style values ----
    void registerStyleNumberTests(test::TestRunner& runner);
    void registerStyleColorTests(test::TestRunner& runner);
    void registerStylePaintTests(test::TestRunner& runner);
    void registerStyleMotionTests(test::TestRunner& runner);
    void registerStyleTrackTests(test::TestRunner& runner);

    // ---- Разметка и компоновка / Markup and layout ----
    void registerMarkupTests(test::TestRunner& runner);
    void registerLayoutStackTests(test::TestRunner& runner);
    void registerLayoutGridTests(test::TestRunner& runner);

    // ---- Текст и файлы / Text and files ----
    void registerUtf8PngTests(test::TestRunner& runner);
    void registerMappedFileTests(test::TestRunner& runner);
    void registerTextSelectionTests(test::TestRunner& runner);
    void registerAccessibilityTests(test::TestRunner& runner);
    void registerPasswordMaskTests(test::TestRunner& runner);
    void registerTextAreaTests(test::TestRunner& runner);
    void registerRichTextTests(test::TestRunner& runner);
    void registerPopupTests(test::TestRunner& runner);
    void registerDialogTests(test::TestRunner& runner);
    void registerDataListTests(test::TestRunner& runner);
    void registerListBindingTests(test::TestRunner& runner);
    void registerWebViewTests(test::TestRunner& runner);
    void registerIncrementalLayoutTests(test::TestRunner& runner);

    // ---- Пример chat: JSON, chunked, NDJSON / The chat example: JSON, chunked, NDJSON ----
    void registerChatJsonTests(test::TestRunner& runner);
    void registerChatStreamTests(test::TestRunner& runner);
    void registerChatMarkdownTests(test::TestRunner& runner);
}

#endif
