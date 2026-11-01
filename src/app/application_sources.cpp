#include "gran/app/internal/app_state.hpp"

namespace gran
{
    void Application::setFilterExpression(const std::string& view, const std::string& expression)
    {
        _state->lists.setFilterExpression(view, expression);
    }

    // Файл читается до замены: при ошибке модель остаётся прежней / The file is read before the replacement: on an error the model stays as it was
    void Application::loadData(const std::string& name, const data::SourceSpec& spec)
    {
        data::loadList(_state->lists.model(name), spec);
    }

    void Application::loadTree(const std::string& name, const data::SourceSpec& spec)
    {
        std::vector<data::TreeItem> nodes = data::readTreeSource(spec);
        data::TreeModel& target = _state->lists.tree(name);

        target.clear();
        data::fillTree(target, target.root(), std::move(nodes));
    }
}
