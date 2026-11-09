#include "gran/ui/node.hpp"

namespace gran::ui
{
    namespace
    {
        // Тексты видимых потомков по порядку через пробел / The visible descendant texts in order by spaces
        void appendStatus(const Node& node, std::string& out)
        {
            for (const std::unique_ptr<Node>& child : node.children)
            {
                if (!child->visible || isLayer(*child))
                    continue;
                if (!child->content.text.empty() && !out.empty())
                    out += ' ';
                out += child->content.text;
                appendStatus(*child, out);
            }
        }

        /*
        ** Обход видимого дерева вне слоёв: у каждой секции строки состояния текст сверяется с объявленным.
        ** Первое чтение только запоминает текст — при загрузке окна диктор не перечисляет всю строку состояния.
        **
        ** A walk over the visible tree outside the layers: for every status bar section the text is compared with the announced one.
        ** The first reading only remembers the text — on loading a window the screen reader does not list the whole status bar.
        */
        void visitStatus(Node& node, std::vector<Node*>& changed)
        {
            if (!node.visible || isLayer(node))
                return;
            if (node.kind == ElementKind::STATUS_SECTION)
            {
                std::string text = statusText(node);
                if (node.content.announcedReady && text != node.content.announced)
                    changed.push_back(&node);
                node.content.announced = std::move(text);
                node.content.announcedReady = true;
                return;
            }
            for (const std::unique_ptr<Node>& child : node.children)
                visitStatus(*child, changed);
        }
    }

    // Ручка строится движком: роль окна «resize», стиль темы SizeGrip, рисует её сцена / The grip is built by the engine: the window role "resize", the theme style SizeGrip, the scene draws it
    void buildSizeGrip(Node& bar)
    {
        std::unique_ptr<Node> grip = std::make_unique<Node>();

        grip->kind = ElementKind::SIZE_GRIP;
        grip->line = bar.line;
        grip->column = bar.column;
        grip->content.windowRole = platform::WindowRole::RESIZE;
        appendChild(bar, std::move(grip));
    }

    // Подписи потомков секции, как их видит пользователь / The section descendant labels as the user sees them
    std::string statusText(const Node& section)
    {
        std::string text = section.content.text;

        appendStatus(section, text);
        return text;
    }

    // Порядок секций — порядок дерева / The section order is the tree order
    void collectStatusChanges(Node& root, std::vector<Node*>& changed)
    {
        visitStatus(root, changed);
    }
}
