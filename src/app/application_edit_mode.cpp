#include "gran/app/internal/app_state.hpp"
#include "gran/markup/overlay.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran
{
    namespace
    {
        // Правки из панели, включая сброс, используют общий шаг отмены.
        // Edits from the panel, including reset, share the same undo path.
        void applyLayoutChange(Application& app, Application::State& state, const std::string& xml)
        {
            if (state.recordLayoutEdit)
                throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::OVERLAY_DENIED, {"layout"}));
            auto name = state.currentProfile.name;
            if (name.empty()) name = "default";
            state.recordLayoutEdit = app.editMode();
            try { app.importProfile(name, xml); }
            catch (...) { state.recordLayoutEdit = false; throw; }
            state.recordLayoutEdit = false;
        }
    }

    // Режим меняет ввод и подсветку основного окна.
    // Edit mode changes input and highlighting in the main window.
    void Application::setEditMode(bool enabled)
    {
        if (enabled == _state->editMode)
            return;
        if (!enabled && !_state->windows.empty() && !app::layoutInspector(*_state->windows.front()))
            app::cancelStylePreview(*_state);
        if (!enabled && !_state->windows.empty())
            app::closeLayoutInspector(*_state->windows.front());
        _state->editMode = enabled;
        _state->editDragging = false;
        _state->editGrip = {};
        _state->editDrop.clear();
        _state->editSelection.clear();
        if (!_state->windows.empty())
            _state->windows.front()->changed = true;
    }

    // Ввод и отрисовка используют один флаг режима правки.
    // Input and painting share the edit-mode flag.
    bool Application::editMode() const
    {
        return _state->editMode;
    }

    // Шаг отмены записывается после успешной установки проверенного документа.
    // Record the undo step after the validated document has been installed.
    void Application::editLayout(const std::string& operation, const markup::TokenMap& attributes, const markup::TokenMap& parameters)
    {
        if (!editMode())
            throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::OVERLAY_DENIED, {operation}));
        const auto& profile = _state->currentProfile;
        auto xml = markup::appendOverlayEdit(profile.effectiveXml(), operation, attributes, parameters);
        applyLayoutChange(*this, *_state, xml);
    }

    // Повторное применение базы удаляет все правки или только операции указанной области.
    // Replaying the base removes all edits or only the operations of the specified region.
    void Application::resetProfile(const std::string& scope)
    {
        const auto& profile = _state->currentProfile;
        if (!profile.base || _state->windows.empty())
            return;
        std::string xml = "<Overlay version=\"1\"/>";
        if (!scope.empty())
        {
            const auto& current = _state->windows.front()->document;
            auto base = markup::loadMarkupString(*profile.base, {current.baseDirectory});
            xml = markup::resetOverlayScope(base, current, profile.xml, scope);
        }
        applyLayoutChange(*this, *_state, xml);
    }
}
