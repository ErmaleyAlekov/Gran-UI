#include "gran/app/internal/app_state.hpp"
#include "gran/core/i18n.hpp"
#include "gran/log/log.hpp"
#include "gran/markup/live_document.hpp"
#include "gran/markup/overlay.hpp"

namespace gran
{
    // Новый наблюдатель заменяет прежнего; пустой снимает подписку.
    // A new observer replaces the previous one; an empty observer removes it.
    void Application::onLayoutChanged(Task handler)
    {
        _state->layoutChanged = std::move(handler);
    }

    // Пустой список правок — действительный исходный профиль для предпросмотра и отмены.
    // An empty operation list is a valid baseline for preview and undo.
    const std::string& Application::State::ProfileChange::effectiveXml() const
    {
        static const std::string EMPTY = "<Overlay version=\"1\"/>";
        if (xml.empty()) return EMPTY;
        return xml;
    }
}

namespace gran::app
{
    // Собираем документ отдельно и передаём готовое дерево обычному механизму перезагрузки.
    // Build a separate document and pass the complete tree to the normal reload mechanism.
    void stageProfileXml(Application::State& state, Application::State::ProfileChange change, std::string_view xml)
    {
        const auto& current = state.windows.front()->document;
        if (!change.base)
            change.base = std::make_shared<const std::string>(markup::serializeMarkup(current));
        markup::LoadOptions options{current.baseDirectory};
        auto base = markup::loadMarkupString(*change.base, options);
        auto result = markup::applyOverlay(base, xml, options);
        for (const auto& id : result.missingIds)
            log::write(log::Level::WARNING, "markup", i18n::format(i18n::Message::LINT_REMOVED_ID, {id}));
        stageDocument(state, std::move(result.document), state.sourcePath, std::move(change));
    }

    // Временное значение меняет окно без записи профиля и шага отмены.
    // A transient value changes the window without persisting a profile or an undo step.
    void previewStyle(Application::State& state, const markup::TokenMap& values)
    {
        if (state.windows.empty() || (state.pendingDocument && !state.pendingProfile.preview)) return;
        auto change = state.currentProfile;
        change.preview = true;
        const auto xml = markup::appendOverlayEdit(change.effectiveXml(), "Token", values);
        stageProfileXml(state, std::move(change), xml);
    }

    // При завершении правки возвращаем последнее подтверждённое наложение.
    // Restore the last committed overlay when editing finishes.
    void cancelStylePreview(Application::State& state)
    {
        if (state.windows.empty() || (state.pendingDocument && !state.pendingProfile.preview)) return;
        if (!state.previewActive && !state.pendingProfile.preview) return;
        auto change = state.currentProfile;
        change.persist = false;
        change.undoXml.clear();
        const auto xml = change.effectiveXml();
        stageProfileXml(state, std::move(change), xml);
    }
}
