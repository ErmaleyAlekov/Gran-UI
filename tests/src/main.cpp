#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/core/i18n.hpp"

namespace
{
    // Функция регистрации одного набора тестов
    // The registration function of one test suite
    using SuiteRegistrar = void (*)(test::TestRunner&);

    // Все наборы в порядке запуска: новый набор достаточно добавить сюда
    // All suites in the run order: a new suite only needs to be added here
    const SuiteRegistrar SUITES[] = {
        suites::registerCapiTests, suites::registerVersionTests,
        suites::registerI18nTests, suites::registerGeometryTests, suites::registerUriListTests, suites::registerReloadTests, suites::registerLiveAppTests, suites::registerComponentTests, suites::registerExpressionTests, suites::registerBuiltinCommandTests, suites::registerWindowOwnerTests, suites::registerTaskPoolTests, suites::registerLogTests, suites::registerResourceTests, suites::registerSettingsTests, suites::registerShortcutTests, suites::registerCommandTests, suites::registerUndoStackTests, suites::registerTextUndoTests, suites::registerAppUndoTests, suites::registerPixelCompareTests, suites::registerAdapterChoiceTests,
        suites::registerWindowFrameTests, suites::registerPlatformFrameTests, suites::registerStyleNumberTests, suites::registerStyleColorTests,
        suites::registerStylePaintTests, suites::registerStyleMotionTests,
        suites::registerStyleTrackTests, suites::registerMarkupTests,
        suites::registerLayoutStackTests, suites::registerLayoutGridTests, suites::registerUtf8PngTests, suites::registerJpegTests, suites::registerVectorShapeTests,
        suites::registerMappedFileTests, suites::registerTextSelectionTests, suites::registerAccessibilityTests, suites::registerAccessActionTests,
        suites::registerPasswordMaskTests, suites::registerTextAreaTests, suites::registerRichTextTests, suites::registerPopupTests, suites::registerMenuBarTests, suites::registerMenuAccessTests, suites::registerToolBarTests, suites::registerStatusBarTests, suites::registerSplitterTests, suites::registerDocumentTabsTests, suites::registerDockTests, suites::registerFrameRebuildTests, suites::registerDockFloatTests, suites::registerWin32KeyMenuTests, suites::registerDialogTests, suites::registerWidgetTests, suites::registerEventRoutingTests, suites::registerEventInputTests, suites::registerDataListTests, suites::registerModelSignalTests, suites::registerSelectionModelTests, suites::registerTreeSelectionTests, suites::registerSortFilterTests, suites::registerDataSourceTests, suites::registerModelMarkupTests, suites::registerListBindingTests, suites::registerWebViewTests, suites::registerIncrementalLayoutTests, suites::registerLayoutCacheTests, suites::registerTimerQueueTests, suites::registerKeyframeTests, suites::registerKeyframeAnimatorTests, suites::registerChatJsonTests, suites::registerChatStreamTests, suites::registerChatMarkdownTests
    };
}

/*
** Точка входа тестов: регистрирует все наборы и запускает их.
** Код возврата 0 — всё прошло, 1 — есть упавшие тесты.
**
** The test entry point: registers all suites and runs them.
** Exit code 0 — everything passed, 1 — there are failed tests.
*/
int main()
{
    test::TestRunner runner;

    test::enableUtf8Console();
    // Тексты ошибок в тестах сверяются по-русски — независимо от языка сборки
    // Tests compare error texts in Russian — regardless of the build language
    gran::i18n::setLanguage("ru");
    for (SuiteRegistrar registrar : SUITES)
        registrar(runner);
    if (runner.runAll() != 0)
        return 1;
    return 0;
}
