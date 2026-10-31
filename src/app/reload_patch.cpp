#include "gran/app/internal/app_state.hpp"
#include "gran/markup/live_document.hpp"

#include "beresta.hpp"

#include <utility>
#include <vector>

namespace gran::app
{
    namespace
    {
        /*
        ** Сначала строятся все новые ветви: построение может бросить (предел узлов), и тогда окно
        ** не тронуто. Ресурсы старого и нового документа равны, поэтому ветвь из нового элемента
        ** со стилями старого документа выходит такой же, как в новом документе.
        **
        ** All the new branches are built first: building may throw (the node limit), and then the window
        ** is untouched. The old and new document resources are equal, so a branch from a new element
        ** with the old document styles comes out the same as in the new document.
        */
        bool buildBranches(AppWindow& win, const markup::PatchPlan& plan, std::vector<std::unique_ptr<ui::Node>>& branches)
        {
            try
            {
                for (const std::pair<ui::Node*, const beresta::Element*>& entry : plan.rebuild)
                    branches.push_back(markup::rebuildFromElement(win.document, *entry.second));
            }
            catch (const std::exception&)
            {
                return false;
            }
            return true;
        }

        // Оставшиеся узлы ссылаются на элементы нового DOM; позиции ошибок — из нового файла
        // The remaining nodes refer to the new DOM elements; the error positions come from the new file
        void relinkNodes(const markup::PatchPlan& plan)
        {
            for (const std::pair<ui::Node*, const beresta::Element*>& entry : plan.relink)
            {
                entry.first->domElement = entry.second;
                entry.first->line = entry.second->getSourceLocation().line;
                entry.first->column = entry.second->getSourceLocation().column;
            }
        }
    }

    /*
    ** Новый DOM становится живым до перестроек, старый живёт до конца функции: перестраиваемые узлы
    ** ещё ссылаются на него, а после перестроек в окне не остаётся ни одной ссылки на старый.
    **
    ** The new DOM becomes live before the rebuilds, the old one lives until the function end: the nodes being rebuilt
    ** still refer to it, and after the rebuilds not a single reference to the old one remains in the window.
    */
    bool patchDocument(AppWindow& win, markup::UiDocument& next)
    {
        markup::PatchPlan plan;
        std::vector<std::unique_ptr<ui::Node>> branches;

        if (next.baseDirectory != win.document.baseDirectory || !markup::planPatch(win.document, next, plan))
            return false;
        if (!buildBranches(win, plan, branches))
            return false;
        std::shared_ptr<beresta::Document> previous = std::move(win.document.dom);
        win.document.dom = next.dom;
        win.document.nodeCount = next.nodeCount;
        relinkNodes(plan);
        for (std::size_t index = 0; index < branches.size(); ++index)
            rebuildInPlace(win, *plan.rebuild[index].first, std::move(branches[index]), true);
        win.changed = true;
        return true;
    }
}
