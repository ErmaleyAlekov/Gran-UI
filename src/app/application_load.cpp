#include "gran/app/internal/app_state.hpp"

#include <filesystem>
#include <utility>

namespace gran::app
{
    /*
    ** Новый документ заменяет прежний документ окна только после успешной загрузки; привязки
    ** прежнего снимаются до замены (они ссылаются на его узлы), привязки других окон остаются.
    ** Затем: начальные состояния, стиль окна, шрифты <Font> относительно каталога разметки,
    ** данные, команды (до списков: копии шаблонов берутся уже привязанными) и привязки списков. Аниматор начинается заново с ключевыми кадрами нового документа.
    ** Подписи ComboBox — после привязок (пункты могут прийти из данных).
    **
    ** A new document replaces the former window document only after a successful load; the bindings
    ** of the former one are dropped before the replacement (they refer to its nodes), the bindings of the other windows stay.
    ** Then: the initial states, the window style, the <Font> fonts relative to the markup directory,
    ** the data, the commands (before the lists: the template copies are taken already bound) and the list bindings. The animator starts anew with the keyframes of the new document.
    ** The ComboBox labels — after the bindings (the items may come from data).
    */
    void adoptDocument(AppWindow& win, markup::UiDocument document, const std::string& base)
    {
        Application::State& state = win.shared;

        if (win.document.root)
        {
            // Узлы прежнего документа уходят: обработчик, во время которого это случилось, прекращает доставку
            // The former document nodes go away: the handler during which this happened stops the delivery
            state.treeVersion++;
            state.lists.forget(*win.document.root);
            forgetWebViews(win, *win.document.root);
        }
        win.document = std::move(document);
        // Анимации прежнего документа ссылались на его узлы / The former document animations referred to its nodes
        win.animator = animation::Animator();
        win.animator.setStyleSheet(&win.document.styles);
        if (win.document.root)
            ui::applyInitialStates(*win.document.root);
        win.windowStyle = markup::windowStyle(markup::defaultTheme(), win.document.window);
        for (const markup::FontFace& face : win.document.fonts)
            state.fonts.addFile((std::filesystem::path(base) / face.source).string());
        state.lists.adoptData(win.document.data);
        applyCommandStates(win);
        if (!win.document.root)
            return;
        bindCommands(win, *win.document.root);
        state.lists.bind(*win.document.root);
        ui::syncComboBoxes(*win.document.root);
    }
}
