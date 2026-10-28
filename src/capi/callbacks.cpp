#include "gran/capi/internal/bridge.hpp"

namespace gran::capi
{
    // Копии строк переживают удаление узла самим callback; наружу не передаётся Node*.
    // String copies survive node removal by the callback; no Node* crosses the ABI.
    void Action::deliver(ui::Node& node, const std::string& message) const
    {
        if (callback == nullptr) return;
        std::string target = node.id;
        std::string textValue = node.content.text;
        std::string option = node.content.option;
        std::string payload = message;
        std::string modelName;
        ItemRef item = app->application.itemOf(node);
        if (item.model != nullptr)
        {
            for (const ui::Node* parent = node.parent; parent != nullptr; parent = parent->parent)
            {
                if (!parent->content.items.empty()) { modelName = parent->content.items; break; }
            }
        }
        gran_event event{target.c_str(), modelName.c_str(), SIZE_MAX, textValue.c_str(), option.c_str(),
            payload.c_str(), node.content.value, 0};
        if (item.model != nullptr) event.index = item.index;
        if (node.content.checked) event.checked = 1;
        callback(user, &event);
    }

    void Action::operator()(ui::Node& node) const { deliver(node, ""); }
    void Action::operator()(ui::Node& node, const std::string& message) const { deliver(node, message); }
    void Task::operator()() const { callback(user); }
}
