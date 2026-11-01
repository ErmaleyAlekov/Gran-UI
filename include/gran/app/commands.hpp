#ifndef GRAN_APP_COMMANDS_HPP
#define GRAN_APP_COMMANDS_HPP
#include "gran/app/application.hpp"

namespace gran::app
{
    // Общие операции для C API и встроенных действий; вся логика остаётся в ядре.
    // Shared operations for the C API and built-in actions; all logic stays in the core.
    ui::Node& requireNode(Application& app, const std::string& id);
    void writeProperty(Application& app, ui::Node& node, const std::string& property, const std::string& value);
    std::string readProperty(const ui::Node& node, const std::string& property);
    void executeCommand(Application& app, const std::string& command, const std::string& target, const std::string& argument);
    bool executeBuiltin(Application::State& state, const std::string& action, ui::Node& node);

    // gran:expand, gran:collapse, gran:toggle у строки плоского дерева (app_list_input.cpp); true — выполнено / gran:expand, gran:collapse, gran:toggle at a flat tree row (app_list_input.cpp); true — done
    bool treeRowBuiltin(Application::State& state, const std::string& command, ui::Node& node);
}
#endif
