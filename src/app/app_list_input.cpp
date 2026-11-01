#include "gran/app/commands.hpp"
#include "gran/app/internal/app_state.hpp"

namespace gran::app
{
    namespace
    {
        // Клавиша платформы → клавиша навигации списка; false — клавиша списку не нужна / A platform key → a list navigation key; false — the list does not need the key
        bool listKeyOf(platform::Key key, bool control, binding::ListKey& result) noexcept
        {
            static const struct { platform::Key key; binding::ListKey list; } KEYS[] = {
                {platform::Key::UP, binding::ListKey::UP}, {platform::Key::DOWN, binding::ListKey::DOWN},
                {platform::Key::HOME, binding::ListKey::HOME}, {platform::Key::END, binding::ListKey::END},
                {platform::Key::PAGE_UP, binding::ListKey::PAGE_UP}, {platform::Key::PAGE_DOWN, binding::ListKey::PAGE_DOWN},
                {platform::Key::LEFT, binding::ListKey::LEFT}, {platform::Key::RIGHT, binding::ListKey::RIGHT},
                {platform::Key::SPACE, binding::ListKey::SPACE}};

            if (key == platform::Key::A && control)
            {
                result = binding::ListKey::SELECT_ALL;
                return true;
            }
            for (const auto& entry : KEYS)
            {
                if (entry.key == key)
                {
                    result = entry.list;
                    return true;
                }
            }
            return false;
        }
    }

    void AppWindow::listPress(ui::Node& container, ui::Node& hit, std::uint8_t modifiers)
    {
        shared.lists.pressItem(container, hit, (modifiers & platform::MODIFIER_CONTROL) != 0,
            (modifiers & platform::MODIFIER_SHIFT) != 0);
    }

    bool AppWindow::listKey(ui::Node& container, platform::Key key, std::uint8_t modifiers)
    {
        bool control = (modifiers & platform::MODIFIER_CONTROL) != 0;
        binding::ListKey list = binding::ListKey::UP;

        if ((modifiers & platform::MODIFIER_ALT) != 0 || !listKeyOf(key, control, list))
            return false;
        return shared.lists.keyItem(container, list, control, (modifiers & platform::MODIFIER_SHIFT) != 0);
    }

    // Узел — внутри копии строки плоского дерева (кнопка-раскрывашка в шаблоне) / The node is inside a flat tree row copy (an expander button in the template)
    bool treeRowBuiltin(Application::State& state, const std::string& command, ui::Node& node)
    {
        if (command != "expand" && command != "collapse" && command != "toggle")
            return false;
        binding::ItemRef item = state.lists.itemOf(node);
        data::TreeList* rows = dynamic_cast<data::TreeList*>(item.model);
        if (rows == nullptr)
            return true;
        if (command == "expand")
            rows->expand(item.index);
        else if (command == "collapse")
            rows->collapse(item.index);
        else
            rows->toggle(item.index);
        return true;
    }
}
