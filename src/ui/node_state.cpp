#include "gran/ui/node_state.hpp"

namespace gran::ui
{
    namespace
    {
        // Виды с состоянием пользователя; остальные узлы в снимок не попадают / The kinds with a user state; the other nodes do not get into the snapshot
        bool keepsState(ElementKind kind) noexcept
        {
            switch (kind)
            {
                case ElementKind::INPUT: case ElementKind::TEXT_AREA: case ElementKind::CHECKBOX:
                case ElementKind::RADIO_BUTTON: case ElementKind::SWITCH: case ElementKind::SLIDER:
                case ElementKind::PROGRESS_BAR: case ElementKind::COMBO_BOX: case ElementKind::TABS:
                case ElementKind::SCROLL:
                    return true;
                default:
                    return false;
            }
        }

        // Состояние одного узла: копируется всё нужное, неважное для вида просто не читается при возврате
        // One node state: everything needed is copied, what does not matter for the kind is simply not read on restoring
        SavedState saveNode(const Node& node)
        {
            SavedState saved;

            saved.kind = node.kind;
            saved.text = node.content.text;
            saved.caret = node.content.caret;
            saved.anchor = node.content.anchor;
            saved.history = node.content.history;
            saved.checked = node.content.checked;
            saved.value = node.content.value;
            saved.option = node.content.option;
            saved.scroll = node.layout.scrollOffset;
            return saved;
        }

        // Обход в глубину с ключами; key — ключ самого узла / A depth-first walk with keys; key is the key of the node itself
        void captureNode(const Node& node, const std::string& key, const Node* focused, TreeState& state)
        {
            if (keepsState(node.kind))
                state.nodes[key] = saveNode(node);
            if (&node == focused)
            {
                state.focus = key;
                state.hasFocus = true;
            }
            for (std::size_t index = 0; index < node.children.size(); ++index)
            {
                const Node& child = *node.children[index];
                if (!child.id.empty())
                    captureNode(child, "#" + child.id, focused, state);
                else
                    captureNode(child, key + "/" + std::to_string(index), focused, state);
            }
        }
    }

    // Ключ корня — его id или пустая строка: снимок и возврат начинают с одного и того же корня
    // The root key is its id or an empty string: the snapshot and the restoring start from the same root
    TreeState captureState(const Node& subtree, const Node* focused)
    {
        TreeState state;

        if (subtree.id.empty())
            captureNode(subtree, std::string(), focused, state);
        else
            captureNode(subtree, "#" + subtree.id, focused, state);
        return state;
    }
}
