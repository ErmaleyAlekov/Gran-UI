#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "gran/animation/animator.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/markup/markup_loader.hpp"
#include "gran/markup/theme.hpp"
#include "gran/style/style_sheet.hpp"
#include "gran/ui/node.hpp"
#include "gran/ui/style_pass.hpp"

namespace
{
    // box играет fade при загрузке, hover — в состоянии Hover / box plays fade on load, hover — in the Hover state
    const char* MARKUP = R"(
<Window>
    <Resources>
        <Keyframes name="fade">
            <Frame at="0%" opacity="0"/>
            <Frame at="100%" opacity="1"/>
        </Keyframes>
        <Style name="Animated" animation="fade 1s linear"/>
    </Resources>
    <Stack>
        <Box id="box" style="Animated" width="10" height="10"/>
        <Box id="hover" width="10" height="10">
            <Hover animation="fade 1s linear"/>
        </Box>
    </Stack>
</Window>
)";

    // Документ и аниматор одного сценария / The document and the animator of one scenario
    struct Scene
    {
        gran::markup::UiDocument doc = gran::markup::loadMarkupString(MARKUP, gran::markup::LoadOptions{});
        gran::animation::Animator animator;

        Scene()
        {
            animator.setStyleSheet(&doc.styles);
        }

        // Проход стилей в момент time / A style pass at the moment time
        void resolve(double time)
        {
            animator.setTime(time);
            gran::ui::resolveStyles(*doc.root, gran::markup::windowStyle(gran::markup::defaultTheme(), doc.window),
                gran::markup::defaultTheme().kinds, &animator);
        }

        // Шаг анимаций в момент time / An animation step at the moment time
        bool tick(double time)
        {
            animator.setTime(time);
            return animator.tick();
        }

        gran::ui::Node& node(const char* id)
        {
            return *gran::ui::findNode(*doc.root, id);
        }
    };

    // Анимация при загрузке: начало сразу, середина, возврат к базе после конца / An animation on load: the start at once, the middle, back to the base after the end
    void testPlaysOnLoad()
    {
        Scene scene;

        scene.resolve(0.0);
        TEST_ASSERT_NEAR(scene.node("box").computed.opacity, 0.0f, 0.01f);
        TEST_ASSERT(scene.tick(0.5));
        TEST_ASSERT_NEAR(scene.node("box").computed.opacity, 0.5f, 0.01f);
        TEST_ASSERT(!scene.tick(1.5));
        TEST_ASSERT_NEAR(scene.node("box").computed.opacity, 1.0f, 0.01f);
    }

    // Состояние Hover запускает свою анимацию / The Hover state starts its animation
    void testStartsByState()
    {
        Scene scene;
        gran::ui::Node& hover = scene.node("hover");

        scene.resolve(0.0);
        TEST_ASSERT_NEAR(hover.computed.opacity, 1.0f, 0.01f);
        hover.stateMask = gran::style::stateBit(gran::style::State::HOVER);
        gran::ui::markDirty(hover, gran::ui::DIRTY_STYLE);
        scene.resolve(2.0);
        TEST_ASSERT_NEAR(hover.computed.opacity, 0.0f, 0.01f);
        TEST_ASSERT(scene.tick(2.25));
        TEST_ASSERT_NEAR(hover.computed.opacity, 0.25f, 0.01f);
    }

    // Закончившаяся анимация не перезапускается пересчётом стиля / A finished animation is not restarted by a style recompute
    void testNoRestart()
    {
        Scene scene;
        gran::ui::Node& box = scene.node("box");

        scene.resolve(0.0);
        TEST_ASSERT(!scene.tick(1.5));
        gran::ui::markDirty(box, gran::ui::DIRTY_STYLE);
        scene.resolve(2.0);
        TEST_ASSERT_NEAR(box.computed.opacity, 1.0f, 0.01f);
        TEST_ASSERT(!scene.tick(2.1));
    }

    // Скрытый узел не держит кадры; при показе анимация продолжается / A hidden node keeps no frames; when shown the animation goes on
    void testHiddenNode()
    {
        Scene scene;
        gran::ui::Node& box = scene.node("box");

        scene.resolve(0.0);
        box.visible = false;
        TEST_ASSERT(!scene.tick(0.5));
        box.visible = true;
        TEST_ASSERT(scene.tick(0.6));
        TEST_ASSERT_NEAR(box.computed.opacity, 0.6f, 0.01f);
    }

    // Удалённый узел забыт аниматором / A removed node is forgotten by the animator
    void testForget()
    {
        Scene scene;

        scene.resolve(0.0);
        scene.animator.forget(scene.node("box"));
        TEST_ASSERT(!scene.tick(0.5));
    }

    // Необъявленное имя ключевых кадров — ошибка загрузки / An undeclared keyframe name is a load error
    void testUnknownName()
    {
        TEST_ASSERT_THROWS(gran::markup::loadMarkupString(R"(<Window><Box animation="nope 1s"/></Window>)",
            gran::markup::LoadOptions{}), gran::MarkupException);
    }
}

namespace suites
{
    // Регистрация набора / The suite registration
    void registerKeyframeAnimatorTests(test::TestRunner& runner)
    {
        runner.add("animation: plays on load", testPlaysOnLoad);
        runner.add("animation: starts by state", testStartsByState);
        runner.add("animation: no restart", testNoRestart);
        runner.add("animation: hidden node", testHiddenNode);
        runner.add("animation: forget", testForget);
        runner.add("animation: unknown keyframes", testUnknownName);
    }
}
