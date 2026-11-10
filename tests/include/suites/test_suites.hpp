#ifndef GRAN_TEST_SUITES_HPP
#define GRAN_TEST_SUITES_HPP

#include "framework/test_framework.hpp"

namespace suites
{
    void registerCapiTests(test::TestRunner& runner);
    void registerVersionTests(test::TestRunner& runner);
    // ---- Ядро и утилиты / Core and utilities ----
    void registerI18nTests(test::TestRunner& runner);
    void registerGeometryTests(test::TestRunner& runner);
    void registerUriListTests(test::TestRunner& runner);

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
    void registerJpegTests(test::TestRunner& runner);
    void registerVectorShapeTests(test::TestRunner& runner);
    void registerMappedFileTests(test::TestRunner& runner);
    void registerTextSelectionTests(test::TestRunner& runner);
    void registerAccessibilityTests(test::TestRunner& runner);
    void registerAccessActionTests(test::TestRunner& runner);
    void registerPasswordMaskTests(test::TestRunner& runner);
    void registerTextAreaTests(test::TestRunner& runner);
    void registerRichTextTests(test::TestRunner& runner);
    void registerPopupTests(test::TestRunner& runner);
    void registerMenuBarTests(test::TestRunner& runner);
    void registerMenuAccessTests(test::TestRunner& runner);
    void registerToolBarTests(test::TestRunner& runner);
    void registerStatusBarTests(test::TestRunner& runner);
    void registerSplitterTests(test::TestRunner& runner);
    void registerDocumentTabsTests(test::TestRunner& runner);
    void registerDockTests(test::TestRunner& runner);
    void registerFrameRebuildTests(test::TestRunner& runner);
    void registerDockFloatTests(test::TestRunner& runner);
    void registerWin32KeyMenuTests(test::TestRunner& runner);
    void registerDialogTests(test::TestRunner& runner);
    void registerWidgetTests(test::TestRunner& runner);
    void registerEventRoutingTests(test::TestRunner& runner);
    void registerEventInputTests(test::TestRunner& runner);
    void registerDataListTests(test::TestRunner& runner);
    void registerModelSignalTests(test::TestRunner& runner);
    void registerSelectionModelTests(test::TestRunner& runner);
    void registerTreeSelectionTests(test::TestRunner& runner);
    void registerSortFilterTests(test::TestRunner& runner);
    void registerDataSourceTests(test::TestRunner& runner);
    void registerModelMarkupTests(test::TestRunner& runner);
    void registerListBindingTests(test::TestRunner& runner);
    void registerWebViewTests(test::TestRunner& runner);
    void registerIncrementalLayoutTests(test::TestRunner& runner);
    void registerTimerQueueTests(test::TestRunner& runner);
    void registerKeyframeTests(test::TestRunner& runner);
    void registerKeyframeAnimatorTests(test::TestRunner& runner);
    void registerReloadTests(test::TestRunner& runner);
    void registerLiveAppTests(test::TestRunner& runner);
    void registerComponentTests(test::TestRunner& runner);
    void registerExpressionTests(test::TestRunner& runner);
    void registerBuiltinCommandTests(test::TestRunner& runner);
    void registerWindowOwnerTests(test::TestRunner& runner);
    void registerTaskPoolTests(test::TestRunner& runner);
    void registerLogTests(test::TestRunner& runner);
    void registerResourceTests(test::TestRunner& runner);
    void registerSettingsTests(test::TestRunner& runner);
    void registerShortcutTests(test::TestRunner& runner);
    void registerCommandTests(test::TestRunner& runner);
    void registerUndoStackTests(test::TestRunner& runner);
    void registerTextUndoTests(test::TestRunner& runner);
    void registerAppUndoTests(test::TestRunner& runner);
    void registerPixelCompareTests(test::TestRunner& runner);
    void registerAdapterChoiceTests(test::TestRunner& runner);
    void registerWindowFrameTests(test::TestRunner& runner);
    void registerPlatformFrameTests(test::TestRunner& runner);

    // ---- Пример chat: JSON, chunked, NDJSON / The chat example: JSON, chunked, NDJSON ----
    void registerChatJsonTests(test::TestRunner& runner);
    void registerChatStreamTests(test::TestRunner& runner);
    void registerChatMarkdownTests(test::TestRunner& runner);
}

#endif
