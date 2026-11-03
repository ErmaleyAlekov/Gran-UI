#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/log/log.hpp"

namespace gran
{
    // Источник запоминается только после успешного разбора.
    // The source is remembered only after successful parsing.
    void Application::reload()
    {
        if (_state->sourcePath.empty())
            throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::APP_RELOAD_NO_FILE));
        loadFile(_state->sourcePath);
    }

    // Отмечаем изменение до разбора: неверный файл не разбирается бесконечно.
    // Mark a change before parsing: an invalid file is not parsed endlessly.
    bool Application::reloadIfChanged()
    {
        if (!app::sourceChanged(*_state))
            return false;
        reload();
        return true;
    }

    // Автоматический режим сохраняет текст последней ошибки.
    // Automatic mode preserves the latest error text.
    const std::string& Application::reloadError() const
    {
        return _state->reloadFailure;
    }
}

namespace gran::app
{
    /*
    ** Шрифты проверяются до любых изменений. Первый документ создаёт главное окно; следующий
    ** внутри run() ждёт границы цикла (обработчик не удаляет узел, из которого вызван), вне run()
    ** заменяется сразу. Другие окна (openWindow) перезагрузка не трогает.
    **
    ** The fonts are checked before any change. The first document creates the main window; the next one
    ** inside run() waits for a loop boundary (a handler does not delete the node it is called from), outside run()
    ** it is replaced at once. Reloading does not touch the other windows (openWindow).
    */
    void stageDocument(Application::State& state, markup::UiDocument document, const std::string& path)
    {
        validateFonts(document, sourceBase(path));
        rememberLibraries(state, document);
        state.reloadFailure.clear();
        if (state.windows.empty())
        {
            addWindow(state, std::move(document), sourceBase(path));
            rememberSource(state, path);
            return;
        }
        state.pendingDocument = std::make_unique<markup::UiDocument>(std::move(document));
        state.pendingPath = path;
        if (!state.running)
            applyReload(state);
    }

    // Ошибка автоматической загрузки не завершает цикл работающего окна.
    // An automatic loading error does not terminate the running window loop.
    bool pollReload(Application::State& state)
    {
        if (reloadWait(state, now()) != 0.0)
            return false;
        state.nextReloadCheck = now() + 0.25;
        try
        {
            if (sourceChanged(state))
                stageDocument(state, markup::loadMarkupFile(state.sourcePath, markup::LoadOptions{}), state.sourcePath);
        }
        catch (const std::exception& error)
        {
            state.reloadFailure = error.what();
            log::write(log::Level::SEVERE, "markup", state.reloadFailure);
        }
        return false;
    }
}
