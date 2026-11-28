#include "gran/app/internal/layout_panel.hpp"

namespace gran::app::panel
{

    // Поля палитры и профилей получают обычные узлы Gran с доступными именами.
    // Palette and profile controls use ordinary Gran nodes with accessible names.
    std::string panelXml(Application::State& state)
    {
        Element window("Window");
        window.setAttribute("title", i18n::format(Message::LAYOUT_PROFILE));
        window.setAttribute("width", "460");
        window.setAttribute("height", "660");
        auto& root = child(window, "Stack");
        root.setAttribute("padding", "12");
        auto& tabs = child(root, "Tabs");
        tabs.setAttribute("grow", "1");
        profilePage(tabs, state);
        auto& status = child(child(child(root, "StatusBar"), "StatusSection"), "Text");
        status.setAttribute("id", "feedback");
        std::string xml;
        window.serialize(xml);
        return xml;
    }
}

namespace gran::app
{
    // Слой принадлежит текущему дереву и исчезает вместе с ним при перестроении.
    // The current tree owns the layer and discards it when rebuilt.
    ui::Node* layoutInspector(AppWindow& win)
    {
        for (const auto& child : win.document.root->children)
            if (child->layoutInspector) return child.get();
        return nullptr;
    }

    // Закрытие слоя возвращает фокус в документ.
    // Closing the layer restores focus to the document.
    void closeLayoutInspector(AppWindow& win)
    {
        if (auto* dialog = layoutInspector(win))
        {
            win.shared.owner->hideDialog(*dialog);
            cancelStylePreview(win.shared);
        }
    }

    // Владелец остаётся доступен для перетаскивания, поэтому окно инструментов немодальное.
    // The owner must remain available for dragging, so the tools window is modeless.
    void openLayoutPanel(Application::State& state)
    {
        if (state.windows.empty() || state.layoutPanel)
            return;
        WindowOptions options;
        options.owner = state.windows.front()->document.root.get();
        options.onClosed = [&state](const std::string&)
        {
            state.layoutPanel = nullptr;
        };
        state.layoutPanel = &state.owner->openWindowString(panel::panelXml(state), options);
        refreshLayoutChoices(state);
    }
}
