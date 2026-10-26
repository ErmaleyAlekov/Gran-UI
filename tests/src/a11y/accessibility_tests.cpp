#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/a11y/access_tree.hpp"
#include "gran/ui/node.hpp"
#include <string>

namespace
{
    void testRoleOf()
    {
        gran::ui::Node button;
        button.kind = gran::ui::ElementKind::BUTTON;
        TEST_ASSERT(gran::a11y::roleOf(button) == gran::a11y::Role::BUTTON);

        gran::ui::Node input;
        input.kind = gran::ui::ElementKind::INPUT;
        TEST_ASSERT(gran::a11y::roleOf(input) == gran::a11y::Role::EDIT);

        gran::ui::Node stack;
        stack.kind = gran::ui::ElementKind::STACK;
        TEST_ASSERT(gran::a11y::roleOf(stack) == gran::a11y::Role::GROUP);
    }

    void testAccessibleName()
    {
        gran::ui::Node button;
        button.kind = gran::ui::ElementKind::BUTTON;
        gran::ui::Node& child1 = gran::ui::appendChild(button, std::make_unique<gran::ui::Node>());
        child1.kind = gran::ui::ElementKind::TEXT;
        child1.content.text = "OK";
        gran::ui::Node& child2 = gran::ui::appendChild(button, std::make_unique<gran::ui::Node>());
        child2.kind = gran::ui::ElementKind::TEXT;
        child2.content.text = "Go";
        TEST_ASSERT(gran::a11y::accessibleName(button) == "OK Go");

        gran::ui::Node input;
        input.kind = gran::ui::ElementKind::INPUT;
        input.content.placeholder = "Enter text";
        TEST_ASSERT(gran::a11y::accessibleName(input) == "Enter text");
    }

    void testDescribe()
    {
        gran::ui::Node input;
        input.kind = gran::ui::ElementKind::INPUT;
        input.content.text = "secret";
        input.content.password = true;
        auto info = gran::a11y::describe(input);
        TEST_ASSERT(info.value.empty());

        input.content.password = false;
        info = gran::a11y::describe(input);
        TEST_ASSERT(info.value == "secret");
    }

    void testNavigate()
    {
        gran::ui::Node root;
        gran::ui::Node& child1 = gran::ui::appendChild(root, std::make_unique<gran::ui::Node>());
        gran::ui::Node& child2 = gran::ui::appendChild(root, std::make_unique<gran::ui::Node>());
        child2.visible = false;
        gran::ui::Node& child3 = gran::ui::appendChild(root, std::make_unique<gran::ui::Node>());

        gran::a11y::AccessTree tree;
        tree.root = &root;

        TEST_ASSERT(gran::a11y::navigate(tree, child1, gran::a11y::Direction::NEXT) == &child3);
        TEST_ASSERT(gran::a11y::navigate(tree, root, gran::a11y::Direction::FIRST_CHILD) == &child1);
        TEST_ASSERT(gran::a11y::navigate(tree, root, gran::a11y::Direction::PARENT) == nullptr);
    }

    void testContains()
    {
        gran::ui::Node root;
        gran::ui::Node& child1 = gran::ui::appendChild(root, std::make_unique<gran::ui::Node>());
        gran::ui::Node& child2 = gran::ui::appendChild(root, std::make_unique<gran::ui::Node>());
        child2.visible = false;

        gran::a11y::AccessTree tree;
        tree.root = &root;

        TEST_ASSERT(!gran::a11y::contains(tree, &child2));
        TEST_ASSERT(gran::a11y::contains(tree, &child1));
    }
}

namespace suites
{
    void registerAccessibilityTests(test::TestRunner& runner)
    {
        runner.add("a11y: roleOf", testRoleOf);
        runner.add("a11y: accessibleName", testAccessibleName);
        runner.add("a11y: describe", testDescribe);
        runner.add("a11y: navigate", testNavigate);
        runner.add("a11y: contains", testContains);
    }
}
