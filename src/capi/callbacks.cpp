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
        size_t index = SIZE_MAX;
        listPosition(app, &node, modelName, index);
        gran_event event{target.c_str(), modelName.c_str(), index, textValue.c_str(), option.c_str(),
            payload.c_str(), node.content.value, 0};
        if (node.content.checked) event.checked = 1;
        callback(user, &event);
    }

    void Action::operator()(ui::Node& node) const { deliver(node, ""); }
    void Action::operator()(ui::Node& node, const std::string& message) const { deliver(node, message); }
    void Task::operator()() const { callback(user); }
}
