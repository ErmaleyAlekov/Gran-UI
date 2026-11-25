#include "gran/ui/node_state.hpp"
#include "node_state_walk.hpp"

#include <algorithm>

namespace gran::ui
{
    namespace
    {
        std::string legacyStateKey(const std::string& key)
        {
            std::string result;
            for (std::size_t i = 0; i < key.size(); ++i)
            {
                if (key.compare(i, 3, "%25") == 0 || key.compare(i, 3, "%2F") == 0)
                {
                    result += key[i + 2] == '5' ? '%' : '/';
                    i += 2;
                }
                else
                    result += key[i];
            }
            return result;
        }

        // Текст поля вместе с историей; каретка не выходит за конец текста / The field text together with the history; the caret does not go past the text end
        void restoreField(Node& node, const SavedState& saved)
        {
            node.content.text = saved.text;
            node.content.caret = std::min(saved.caret, saved.text.size());
            node.content.anchor = std::min(saved.anchor, saved.text.size());
            node.content.history = saved.history;
        }

        /*
        ** Выбор ComboBox и Tabs — через их функции: они же переключают пункт или страницу и подпись.
        ** Пропавший пункт или вкладка оставляют выбор из разметки.
        **
        ** The ComboBox and Tabs choice goes through their functions: they also switch the item or the page and the label.
        ** A vanished item or tab keeps the choice from the markup.
        */
        void restoreNode(Node& node, const SavedState& saved)
        {
            switch (node.kind)
            {
                case ElementKind::INPUT: case ElementKind::TEXT_AREA:
                    restoreField(node, saved);
                    break;
                case ElementKind::CHECKBOX: case ElementKind::RADIO_BUTTON: case ElementKind::SWITCH:
                    setChecked(node, saved.checked);
                    break;
                case ElementKind::SLIDER: case ElementKind::PROGRESS_BAR:
                    node.content.value = std::min(node.content.maximum, std::max(node.content.minimum, saved.value));
                    break;
                case ElementKind::COMBO_BOX:
                    selectOption(node, findOption(node, saved.option));
                    break;
                case ElementKind::TABS:
                    selectTab(node, saved.option);
                    break;
                default:
                    node.layout.scrollOffset = saved.scroll;
                    break;
            }
            markDirty(node, DIRTY_LAYOUT | DIRTY_PAINT);
        }

        // Те же ключи, что у captureNode; skip — не трогать сам узел (его детей — трогать)
        // The same keys as in captureNode; skip — do not touch the node itself (its children — do)
        void restoreWalk(Node& node, const std::string& key, const TreeState& state, bool skip, Node*& focus)
        {
            std::map<std::string, SavedState>::const_iterator found = state.nodes.find(key);

            if (!skip && found != state.nodes.end() && found->second.kind == node.kind)
                restoreNode(node, found->second);
            if (state.hasFocus && state.focus == key)
                focus = &node;
        }
    }

    // Узел для фокуса ищется тем же обходом: ключ фокуса сохранился, если узел остался на месте
    // The node to focus is looked for by the same walk: the focus key survived if the node stayed in place
    Node* restoreState(Node& subtree, const TreeState& state, bool skipRoot)
    {
        Node* focus = nullptr;

        internal::walkState(subtree, [&](Node& node, const std::string& key, const std::string& path) {
            if (state.pathKeys.empty())
            {
                restoreWalk(node, legacyStateKey(key), state, skipRoot && &node == &subtree, focus);
                return;
            }
            auto previous = state.pathKeys.find(path);
            const bool scoped = previous != state.pathKeys.end() &&
                (key.compare(0, 1, "@") == 0 || previous->second.compare(0, 1, "@") == 0);
            restoreWalk(node, scoped ? previous->second : key, state, skipRoot && &node == &subtree, focus);
        });
        return focus;
    }
}
