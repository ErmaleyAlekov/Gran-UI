#include "gran/app/internal/app_state.hpp"

#include <filesystem>
#include <utility>

namespace gran::app
{
    /*
    ** Новый документ заменяет прежний только после успешной загрузки; привязки прежнего
    ** снимаются до замены (они ссылаются на его узлы). Затем: начальные состояния, стиль окна,
    ** шрифты <Font> относительно каталога разметки, данные и привязки списков.
    **
    ** A new document replaces the former one only after a successful load; the bindings of the former one
    ** are dropped before the replacement (they refer to its nodes). Then: the initial states, the window style,
    ** the <Font> fonts relative to the markup directory, the data and the list bindings.
    ** Аниматор начинается заново с ключевыми кадрами нового документа / The animator starts anew with the keyframes of the new document.
    ** Подписи ComboBox — после привязок (пункты могут прийти из данных) / The ComboBox labels — after the bindings (the items may come from data).
    */
    void adoptDocument(Application::State& state, markup::UiDocument document, const std::string& base)
    {
        state.lists.clear();
        state.webSlots.clear();
        state.webInbox.clear();
        state.document = std::move(document);
        // Анимации прежнего документа ссылались на его узлы; кадры <Keyframes> — из нового
        // The former document animations referred to its nodes; the <Keyframes> come from the new one
        state.animator = animation::Animator();
        state.animator.setStyleSheet(&state.document.styles);
        if (state.document.root)
            ui::applyInitialStates(*state.document.root);
        state.windowStyle = markup::windowStyle(markup::defaultTheme(), state.document.window);
        for (const markup::FontFace& face : state.document.fonts)
            state.fonts.addFile((std::filesystem::path(base) / face.source).string());
        state.lists.adoptData(state.document.data);
        if (!state.document.root)
            return;
        state.lists.bind(*state.document.root);
        ui::syncComboBoxes(*state.document.root);
    }
}
