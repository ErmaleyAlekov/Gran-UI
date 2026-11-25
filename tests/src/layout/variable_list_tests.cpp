#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"
#include "support/layout_helpers.hpp"
#include "gran/binding/list_bindings.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/layout/layout.hpp"
#include "gran/markup/markup_loader.hpp"
#include "gran/markup/theme.hpp"
#include "gran/ui/style_pass.hpp"
#include <string>
#include <vector>

namespace
{
    void settle(gran::markup::UiDocument& doc, gran::binding::ListBindings& lists, support::FixedMeasurer& measurer)
    {
        for (int i = 0; i < 5; ++i)
        {
            gran::ui::resolveStyles(*doc.root, gran::markup::windowStyle(gran::markup::defaultTheme(), doc.window), gran::markup::defaultTheme().kinds);
            gran::layout::layoutTree(*doc.root, gran::Size{400, 300}, measurer);
            if (!lists.updateVirtual())
                break;
        }
    }

    std::vector<std::pair<std::size_t, const gran::ui::Node*>> copies(const gran::ui::Node& list, const gran::binding::ListBindings& lists)
    {
        std::vector<std::pair<std::size_t, const gran::ui::Node*>> result;
        for (const auto& child : list.children)
        {
            auto itemRef = lists.itemOf(*child);
            if (itemRef.model != nullptr)
            {
                result.emplace_back(itemRef.index, child.get());
            }
        }
        return result;
    }

    void testRevealAfterStaticChildren()
    {
        support::FixedMeasurer measurer;
        auto doc = gran::markup::loadMarkupString(R"(<Window><Scroll id="s"><Stack id="list" items="rows" spacing="4" virtual="true"><Box height="80" margin="6"/><Box height="90" visible="false"/><ItemTemplate><Box height="20"/></ItemTemplate></Stack></Scroll></Window>)", {});
        gran::binding::ListBindings lists;
        for (std::size_t index = 0; index < 100; ++index)
            lists.model("rows").append({});
        lists.bind(*doc.root);
        settle(doc, lists, measurer);

        auto* list = gran::ui::findNode(*doc.root, "list");
        auto* scroll = gran::ui::findNode(*doc.root, "s");
        TEST_ASSERT(list != nullptr && scroll != nullptr);
        constexpr std::size_t target = 70;
        TEST_ASSERT(lists.itemNode(*list, target) == nullptr);
        TEST_ASSERT(lists.revealItem(*list, target));
        settle(doc, lists, measurer);

        const auto* item = lists.itemNode(*list, target);
        TEST_ASSERT(item != nullptr);
        const auto& row = item->layout.frame;
        const auto& viewport = scroll->layout.frame;
        TEST_ASSERT(row.y >= viewport.y - 0.5f);
        TEST_ASSERT_NEAR(row.y + row.height, viewport.y + viewport.height, 0.5f);
    }

    void testVariableListCoversViewport()
    {
        support::FixedMeasurer measurer;
        auto doc = gran::markup::loadMarkupString(R"(<Window><Scroll id="s"><Stack id="list" items="rows" spacing="4" virtual="true"><ItemTemplate><Text>{t}</Text></ItemTemplate></Stack></Scroll></Window>)", gran::markup::LoadOptions{});
        gran::binding::ListBindings lists;
        for (std::size_t i = 0; i < 3000; ++i)
        {
            if ((i / 50) % 2 == 0)
            {
                lists.model("rows").append({{"t", "a"}});
            }
            else
            {
                lists.model("rows").append({{"t", std::string(300, 'x')}});
            }
        }
        lists.bind(*doc.root);

        settle(doc, lists, measurer);

        auto* list = gran::ui::findNode(*doc.root, "list");
        TEST_ASSERT(list != nullptr);

        auto* s = gran::ui::findNode(*doc.root, "s");
        TEST_ASSERT(s != nullptr);

        for (auto offset : {0, 20000, 40000, 60000})
        {
            s->layout.scrollOffset.y = offset;
            gran::ui::markDirty(*s, gran::ui::DIRTY_LAYOUT);
            settle(doc, lists, measurer);

            auto copiesList = copies(*list, lists);
            TEST_ASSERT(!copiesList.empty());

            for (std::size_t i = 1; i < copiesList.size(); ++i)
            {
                TEST_ASSERT(copiesList[i].first == copiesList[i - 1].first + 1);
            }

            TEST_ASSERT(copiesList.front().second->layout.frame.y <= s->layout.frame.y);
            TEST_ASSERT(copiesList.back().second->layout.frame.y + copiesList.back().second->layout.frame.height >= s->layout.frame.y + s->layout.frame.height || copiesList.back().first == 2999);
        }
    }

    void testVariableListExactOffsets()
    {
        support::FixedMeasurer measurer;
        auto doc = gran::markup::loadMarkupString(R"(<Window><Scroll id="s"><Stack id="list" items="rows" spacing="4" virtual="true"><ItemTemplate><Text>{t}</Text></ItemTemplate></Stack></Scroll></Window>)", gran::markup::LoadOptions{});
        gran::binding::ListBindings lists;
        for (std::size_t i = 0; i < 3000; ++i)
        {
            if ((i / 50) % 2 == 0)
            {
                lists.model("rows").append({{"t", "a"}});
            }
            else
            {
                lists.model("rows").append({{"t", std::string(300, 'x')}});
            }
        }
        lists.bind(*doc.root);

        settle(doc, lists, measurer);

        auto* list = gran::ui::findNode(*doc.root, "list");
        TEST_ASSERT(list != nullptr);

        auto* s = gran::ui::findNode(*doc.root, "s");
        TEST_ASSERT(s != nullptr);

        // Короткая запись 0 — в начальном окне / The short record 0 is in the initial window
        float hs = copies(*list, lists).front().second->layout.frame.height;

        // Прокрутка до конца шагами: каждая запись побывает в окне и будет измерена
        // Scrolling to the end in steps: every record visits the window and gets measured
        for (int step = 0; step < 3000; ++step)
        {
            float before = s->layout.scrollOffset.y;
            s->layout.scrollOffset.y = before + 250.0f;
            gran::ui::markDirty(*s, gran::ui::DIRTY_LAYOUT);
            settle(doc, lists, measurer);
            if (s->layout.scrollOffset.y <= before + 0.5f)
                break;
        }

        auto copiesList = copies(*list, lists);
        TEST_ASSERT(!copiesList.empty());
        // Последние 50 записей — длинные / The last 50 records are long
        float hl = copiesList.back().second->layout.frame.height;
        TEST_ASSERT(hs > 0.0f && hl > hs);

        // Точное место каждой записи — сумма высот и промежутков перед ней / The exact place of every record is the sum of the heights and gaps before it
        std::vector<double> offsets(3001, 0.0);
        for (std::size_t j = 0; j < 3000; ++j)
        {
            float h = hl;
            if ((j / 50) % 2 == 0)
                h = hs;
            offsets[j + 1] = offsets[j] + h + 4.0;
        }
        float start = list->layout.frame.y + list->computed.padding.top + list->computed.borderWidth.top;
        for (const auto& copy : copiesList)
            TEST_ASSERT_NEAR(copy.second->layout.frame.y - start, static_cast<float>(offsets[copy.first]), 0.5f);
        float boxes = list->computed.padding.top + list->computed.padding.bottom + list->computed.borderWidth.top
            + list->computed.borderWidth.bottom;
        TEST_ASSERT_NEAR(list->layout.frame.height, static_cast<float>(offsets[3000] - 4.0) + boxes, 1.0f);
    }

    void testVariableListAnchor()
    {
        support::FixedMeasurer measurer;
        auto doc = gran::markup::loadMarkupString(R"(<Window><Scroll id="s"><Stack id="list" items="rows" spacing="4" virtual="true"><ItemTemplate><Text>{t}</Text></ItemTemplate></Stack></Scroll></Window>)", gran::markup::LoadOptions{});
        gran::binding::ListBindings lists;
        for (std::size_t i = 0; i < 3000; ++i)
        {
            if ((i / 50) % 2 == 0)
            {
                lists.model("rows").append({{"t", "a"}});
            }
            else
            {
                lists.model("rows").append({{"t", std::string(300, 'x')}});
            }
        }
        lists.bind(*doc.root);

        settle(doc, lists, measurer);

        auto* list = gran::ui::findNode(*doc.root, "list");
        TEST_ASSERT(list != nullptr);

        auto* s = gran::ui::findNode(*doc.root, "s");
        TEST_ASSERT(s != nullptr);

        s->layout.scrollOffset.y = 50000;
        gran::ui::markDirty(*s, gran::ui::DIRTY_LAYOUT);
        settle(doc, lists, measurer);

        auto copiesList = copies(*list, lists);
        TEST_ASSERT(!copiesList.empty());

        std::size_t r = 0;
        while (r < copiesList.size() && copiesList[r].second->layout.frame.y < s->layout.frame.y)
        {
            ++r;
        }
        TEST_ASSERT(r < copiesList.size());

        float y0 = copiesList[r].second->layout.frame.y - s->layout.frame.y;

        for (int step = 0; step < 10; ++step)
        {
            s->layout.scrollOffset.y -= 15;
            gran::ui::markDirty(*s, gran::ui::DIRTY_LAYOUT);
            settle(doc, lists, measurer);

            auto newCopiesList = copies(*list, lists);
            TEST_ASSERT(!newCopiesList.empty());

            auto found = false;
            for (const auto& copy : newCopiesList)
            {
                if (copy.first == copiesList[r].first)
                {
                    TEST_ASSERT_NEAR(copy.second->layout.frame.y - s->layout.frame.y, y0 + 15.0f * static_cast<float>(step + 1), 0.5f);
                    found = true;
                    break;
                }
            }
            TEST_ASSERT(found);
        }
    }

    // Записи блоками по 50: короткие, затем длинные / Records in blocks of 50: short, then long
    void fillRows(gran::binding::ListBindings& lists)
    {
        for (std::size_t i = 0; i < 3000; ++i)
        {
            std::string text = "a";
            if ((i / 50) % 2 != 0)
                text = std::string(300, 'x');
            lists.model("rows").append({{"t", text}});
        }
    }

    // Копия записи record (nullptr — вне окна) / The copy of the record (nullptr — outside the window)
    const gran::ui::Node* copyOf(const gran::ui::Node& list, const gran::binding::ListBindings& lists, std::size_t record)
    {
        for (const auto& copy : copies(list, lists))
        {
            if (copy.first == record)
                return copy.second;
        }
        return nullptr;
    }

    /*
    ** Ревью Т-60: прыжок к самому началу после прокрутки в середину — старый якорь не должен утянуть
    ** прокрутку обратно вниз, когда записи у начала измеряются.
    **
    ** Review T-60: a jump to the very start after scrolling to the middle — the old anchor must not pull
    ** the scroll back down while the records at the start get measured.
    */
    void testVariableListJumpToTop()
    {
        support::FixedMeasurer measurer;
        auto doc = gran::markup::loadMarkupString(R"(<Window><Scroll id="s"><Stack id="list" items="rows" spacing="4" virtual="true"><ItemTemplate><Text>{t}</Text></ItemTemplate></Stack></Scroll></Window>)", gran::markup::LoadOptions{});
        gran::binding::ListBindings lists;
        fillRows(lists);
        lists.bind(*doc.root);
        settle(doc, lists, measurer);
        auto* list = gran::ui::findNode(*doc.root, "list");
        auto* s = gran::ui::findNode(*doc.root, "s");
        TEST_ASSERT(list != nullptr && s != nullptr);

        // Медленно вниз до длинных записей, затем прыжок к началу / Slowly down to the long records, then a jump to the start
        for (int step = 0; step < 40; ++step)
        {
            s->layout.scrollOffset.y += 60.0f;
            gran::ui::markDirty(*s, gran::ui::DIRTY_LAYOUT);
            settle(doc, lists, measurer);
        }
        s->layout.scrollOffset.y = 0.0f;
        gran::ui::markDirty(*s, gran::ui::DIRTY_LAYOUT);
        settle(doc, lists, measurer);

        TEST_ASSERT_NEAR(s->layout.scrollOffset.y, 0.0f, 0.01f);
        const gran::ui::Node* first = copyOf(*list, lists, 0);
        TEST_ASSERT(first != nullptr);
        TEST_ASSERT_NEAR(first->layout.frame.y, s->layout.frame.y, 0.5f);
    }

    /*
    ** Ревью Т-60: правка записи над окном забывает её размер и меняет среднюю оценку — видимая запись
    ** при этом не должна сдвинуться на экране (якорь гасит смену оценки).
    **
    ** Review T-60: editing a record above the window forgets its size and changes the average estimate — the visible
    ** record must not move on the screen meanwhile (the anchor cancels the estimate change).
    */
    void testVariableListEditAboveWindow()
    {
        support::FixedMeasurer measurer;
        auto doc = gran::markup::loadMarkupString(R"(<Window><Scroll id="s"><Stack id="list" items="rows" spacing="4" virtual="true"><ItemTemplate><Text>{t}</Text></ItemTemplate></Stack></Scroll></Window>)", gran::markup::LoadOptions{});
        gran::binding::ListBindings lists;
        fillRows(lists);
        lists.bind(*doc.root);
        settle(doc, lists, measurer);
        auto* list = gran::ui::findNode(*doc.root, "list");
        auto* s = gran::ui::findNode(*doc.root, "s");
        TEST_ASSERT(list != nullptr && s != nullptr);

        // Медленно вниз: записи 0..~60 измерены и ушли из окна / Slowly down: the records 0..~60 are measured and left the window
        for (int step = 0; step < 40; ++step)
        {
            s->layout.scrollOffset.y += 60.0f;
            gran::ui::markDirty(*s, gran::ui::DIRTY_LAYOUT);
            settle(doc, lists, measurer);
        }
        auto before = copies(*list, lists);
        TEST_ASSERT(!before.empty() && before.front().first > 10);
        std::size_t record = before.back().first;
        float y0 = before.back().second->layout.frame.y - s->layout.frame.y;

        lists.model("rows").set(5, "t", std::string(900, 'y'));
        lists.model("rows").set(7, "t", "b");
        settle(doc, lists, measurer);

        const gran::ui::Node* copy = copyOf(*list, lists, record);
        TEST_ASSERT(copy != nullptr);
        TEST_ASSERT_NEAR(copy->layout.frame.y - s->layout.frame.y, y0, 0.5f);
    }
}

namespace suites
{
    void registerVariableListTests(test::TestRunner& runner)
    {
        runner.add("variable list: covers viewport", testVariableListCoversViewport);
        runner.add("variable list: reveal after static children", testRevealAfterStaticChildren);
        runner.add("variable list: exact offsets", testVariableListExactOffsets);
        runner.add("variable list: anchor", testVariableListAnchor);
        runner.add("variable list: jump to top", testVariableListJumpToTop);
        runner.add("variable list: edit above window", testVariableListEditAboveWindow);
    }
}
