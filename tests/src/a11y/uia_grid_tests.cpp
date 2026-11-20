#include "framework/test_framework.hpp"
#include "suites/test_suites.hpp"

#ifdef _WIN32
#include "gran/app/app_driver.hpp"
#include "gran/app/application.hpp"
#include "gran/platform/win32/internal/uia_element.hpp"
#include "gran/platform/headless/headless_window.hpp"
#include "gran/ui/data_grid.hpp"
#include "gran/data/list_model.hpp"
#include <memory>
#include <string>
#include <vector>

namespace
{
    const char* const MARKUP = R"(<Window width="500" height="400"><Resources>
<Tree name="org"><Item name="Бухгалтерия" staff="6"><Item name="Расчёт" staff="2"/><Item name="Налоги" staff="3"/></Item><Item name="Склад" staff="11"/></Tree>
<TreeList name="rows" tree="org"/>
<Selection name="sel" model="rows" mode="single"/>
</Resources>
<Stack><DataGrid id="tg" items="rows" selection="sel" height="200">
<GridColumn field="staff" title="Штат" width="80"/>
<GridColumn field="name" title="Отдел" tree="true" width="1fr"/>
</DataGrid></Stack></Window>)";

    // Тип элемента и целое свойство узла через провайдер / The element type and an integer property of the node through the provider
    long intProperty(const std::shared_ptr<gran::platform::win32::UiaHost>& host, gran::ui::Node* node, PROPERTYID property)
    {
        gran::platform::win32::UiaElement* e = gran::platform::win32::elementFor(host, node);
        VARIANT v;
        long result = -1;
        VariantInit(&v);
        if (e->GetPropertyValue(property, &v) == S_OK && v.vt == VT_I4)
            result = v.lVal;
        VariantClear(&v);
        e->Release();
        return result;
    }

    gran::AppOptions headless()
    {
        gran::AppOptions options;
        options.headless = true;
        options.backend = gran::rhi::BackendKind::NONE;
        return options;
    }

    std::vector<gran::ui::Node*> gridRowsOf(gran::Application& app)
    {
        std::vector<gran::ui::Node*> rows;
        gran::ui::Node* rowsNode = gran::ui::gridRows(*app.find("tg"));
        if (rowsNode == nullptr)
            return rows;
        for (const auto& child : rowsNode->children)
        {
            if (child->kind == gran::ui::ElementKind::GRID_ROW)
                rows.push_back(child.get());
        }
        return rows;
    }
}

#endif

namespace suites
{
    void registerUiaGridTests(test::TestRunner& runner)
    {
#ifdef _WIN32
        runner.add("uia grid: grid and table", []() {
            gran::Application app(headless());
            app.loadString(MARKUP);
            gran::AppDriver driver(app);
            driver.start();
            gran::a11y::AccessTree* tree = driver.window(0).state().accessTree;
            auto host = std::make_shared<gran::platform::win32::UiaHost>();
            host->tree = tree;
            gran::ui::Node* tg = app.find("tg");
            gran::platform::win32::UiaElement* e = gran::platform::win32::elementFor(host, tg);
            IGridProvider* grid = nullptr;
            TEST_ASSERT(e->QueryInterface(__uuidof(IGridProvider), reinterpret_cast<void**>(&grid)) == S_OK);
            int rowCount = 0;
            TEST_ASSERT(grid->get_RowCount(&rowCount) == S_OK);
            TEST_ASSERT(rowCount == 2);
            int columnCount = 0;
            TEST_ASSERT(grid->get_ColumnCount(&columnCount) == S_OK);
            TEST_ASSERT(columnCount == 2);
            IRawElementProviderSimple* item = nullptr;
            TEST_ASSERT(grid->GetItem(1, 1, &item) == S_OK);
            TEST_ASSERT(item != nullptr);
            IGridItemProvider* gridItem = nullptr;
            TEST_ASSERT(item->QueryInterface(__uuidof(IGridItemProvider), reinterpret_cast<void**>(&gridItem)) == S_OK);
            int row = 0;
            TEST_ASSERT(gridItem->get_Row(&row) == S_OK);
            TEST_ASSERT(row == 1);
            int column = 0;
            TEST_ASSERT(gridItem->get_Column(&column) == S_OK);
            TEST_ASSERT(column == 1);
            int span = 0;
            TEST_ASSERT(gridItem->get_RowSpan(&span) == S_OK);
            TEST_ASSERT(span == 1);
            IRawElementProviderSimple* containingGrid = nullptr;
            TEST_ASSERT(gridItem->get_ContainingGrid(&containingGrid) == S_OK);
            TEST_ASSERT(containingGrid != nullptr);
            containingGrid->Release();
            item->Release();
            IRawElementProviderSimple* item2 = nullptr;
            TEST_ASSERT(grid->GetItem(5, 0, &item2) == E_INVALIDARG);
            ITableProvider* table = nullptr;
            TEST_ASSERT(e->QueryInterface(__uuidof(ITableProvider), reinterpret_cast<void**>(&table)) == S_OK);
            SAFEARRAY* array = nullptr;
            TEST_ASSERT(table->GetColumnHeaders(&array) == S_OK);
            LONG upper = 0;
            TEST_ASSERT(SafeArrayGetUBound(array, 1, &upper) == S_OK);
            TEST_ASSERT(upper == 1);
            SafeArrayDestroy(array);
            enum RowOrColumnMajor major;
            TEST_ASSERT(table->get_RowOrColumnMajor(&major) == S_OK);
            TEST_ASSERT(major == RowOrColumnMajor_RowMajor);
            ITableItemProvider* tableItem = nullptr;
            TEST_ASSERT(item->QueryInterface(__uuidof(ITableItemProvider), reinterpret_cast<void**>(&tableItem)) == S_OK);
            TEST_ASSERT(tableItem->GetColumnHeaderItems(&array) == S_OK);
            TEST_ASSERT(SafeArrayGetUBound(array, 1, &upper) == S_OK);
            TEST_ASSERT(upper == 0);
            SafeArrayDestroy(array);
            tableItem->Release();
            table->Release();
            gridItem->Release();
            e->Release();
            host->tree = nullptr;
        });

        runner.add("uia grid: control types", []() {
            gran::Application app(headless());
            app.loadString(MARKUP);
            gran::AppDriver driver(app);
            driver.start();
            gran::a11y::AccessTree* tree = driver.window(0).state().accessTree;
            auto host = std::make_shared<gran::platform::win32::UiaHost>();
            host->tree = tree;
            gran::ui::Node* tg = app.find("tg");
            gran::platform::win32::UiaElement* e = gran::platform::win32::elementFor(host, tg);
            e->Release();
            gran::ui::Node* row = gridRowsOf(app)[0];
            gran::ui::Node* header = gran::ui::gridHeader(*tg);
            TEST_ASSERT(intProperty(host, tg, UIA_ControlTypePropertyId) == UIA_DataGridControlTypeId);
            TEST_ASSERT(intProperty(host, row, UIA_ControlTypePropertyId) == UIA_DataItemControlTypeId);
            TEST_ASSERT(intProperty(host, row->children[0].get(), UIA_ControlTypePropertyId) == UIA_DataItemControlTypeId);
            TEST_ASSERT(intProperty(host, header, UIA_ControlTypePropertyId) == UIA_HeaderControlTypeId);
            TEST_ASSERT(intProperty(host, header->children[0].get(), UIA_ControlTypePropertyId) == UIA_HeaderItemControlTypeId);
            TEST_ASSERT(intProperty(host, row, UIA_LevelPropertyId) == 1);
            TEST_ASSERT(intProperty(host, row, UIA_PositionInSetPropertyId) == 1);
            TEST_ASSERT(intProperty(host, row, UIA_SizeOfSetPropertyId) == 2);
            // Направление сортировки — свойством заголовка (как aria-sort), а не стрелкой в имени (Т-73)
            // The sort direction is a header property (as aria-sort), not an arrow in the name (T-73)
            gran::ui::markSortedColumn(*tg, 1, true);
            gran::platform::win32::UiaElement* sorted = gran::platform::win32::elementFor(host, header->children[1].get());
            VARIANT aria;
            VariantInit(&aria);
            TEST_ASSERT(sorted->GetPropertyValue(UIA_AriaPropertiesPropertyId, &aria) == S_OK && aria.vt == VT_BSTR);
            TEST_ASSERT(aria.vt == VT_BSTR && std::wstring(aria.bstrVal) == L"sort=descending");
            VariantClear(&aria);
            sorted->Release();
            TEST_ASSERT(intProperty(host, header->children[0].get(), UIA_AriaPropertiesPropertyId) == -1);
            host->tree = nullptr;
        });

        runner.add("uia grid: selection", []() {
            gran::Application app(headless());
            app.loadString(MARKUP);
            gran::AppDriver driver(app);
            driver.start();
            gran::a11y::AccessTree* tree = driver.window(0).state().accessTree;
            auto host = std::make_shared<gran::platform::win32::UiaHost>();
            host->tree = tree;
            gran::ui::Node* tg = app.find("tg");
            gran::platform::win32::UiaElement* e = gran::platform::win32::elementFor(host, tg);
            ISelectionProvider* selection = nullptr;
            TEST_ASSERT(e->QueryInterface(__uuidof(ISelectionProvider), reinterpret_cast<void**>(&selection)) == S_OK);
            BOOL multiple = FALSE;
            TEST_ASSERT(selection->get_CanSelectMultiple(&multiple) == S_OK);
            TEST_ASSERT(multiple == FALSE);
            BOOL required = FALSE;
            TEST_ASSERT(selection->get_IsSelectionRequired(&required) == S_OK);
            TEST_ASSERT(required == FALSE);
            gran::platform::win32::UiaElement* line = gran::platform::win32::elementFor(host, gridRowsOf(app)[1]);
            ISelectionItemProvider* item = nullptr;
            TEST_ASSERT(line->QueryInterface(__uuidof(ISelectionItemProvider), reinterpret_cast<void**>(&item)) == S_OK);
            BOOL selected = TRUE;
            TEST_ASSERT(item->get_IsSelected(&selected) == S_OK && selected == FALSE);
            TEST_ASSERT(item->Select() == S_OK);
            driver.settle();
            TEST_ASSERT(item->get_IsSelected(&selected) == S_OK && selected == TRUE);
            SAFEARRAY* array = nullptr;
            LONG upper = -1;
            TEST_ASSERT(selection->GetSelection(&array) == S_OK);
            TEST_ASSERT(SafeArrayGetUBound(array, 1, &upper) == S_OK && upper == 0);
            SafeArrayDestroy(array);
            IRawElementProviderSimple* container = nullptr;
            TEST_ASSERT(item->get_SelectionContainer(&container) == S_OK && container != nullptr);
            container->Release();
            gran::platform::win32::UiaElement* first = gran::platform::win32::elementFor(host, gridRowsOf(app)[0]);
            TEST_ASSERT(first->AddToSelection() == static_cast<HRESULT>(UIA_E_INVALIDOPERATION));
            first->Release();
            item->Release();
            line->Release();
            selection->Release();
            e->Release();
            host->tree = nullptr;
        });

        runner.add("uia grid: expand collapse", []() {
            gran::Application app(headless());
            app.loadString(MARKUP);
            gran::AppDriver driver(app);
            driver.start();
            gran::a11y::AccessTree* tree = driver.window(0).state().accessTree;
            auto host = std::make_shared<gran::platform::win32::UiaHost>();
            host->tree = tree;
            gran::platform::win32::UiaElement* e = gran::platform::win32::elementFor(host, gridRowsOf(app)[0]);
            IExpandCollapseProvider* expandCollapse = nullptr;
            TEST_ASSERT(e->QueryInterface(__uuidof(IExpandCollapseProvider), reinterpret_cast<void**>(&expandCollapse)) == S_OK);
            ExpandCollapseState state;
            TEST_ASSERT(expandCollapse->get_ExpandCollapseState(&state) == S_OK);
            TEST_ASSERT(state == ExpandCollapseState_Collapsed);
            TEST_ASSERT(expandCollapse->Expand() == S_OK);
            driver.settle();
            TEST_ASSERT(gridRowsOf(app).size() == 4);
            TEST_ASSERT(expandCollapse->get_ExpandCollapseState(&state) == S_OK);
            TEST_ASSERT(state == ExpandCollapseState_Expanded);
            gran::platform::win32::UiaElement* leaf = gran::platform::win32::elementFor(host, gridRowsOf(app)[3]);
            TEST_ASSERT(leaf->get_ExpandCollapseState(&state) == S_OK && state == ExpandCollapseState_LeafNode);
            TEST_ASSERT(leaf->Expand() == static_cast<HRESULT>(UIA_E_INVALIDOPERATION));
            leaf->Release();
            TEST_ASSERT(expandCollapse->Collapse() == S_OK);
            driver.settle();
            TEST_ASSERT(gridRowsOf(app).size() == 2);
            expandCollapse->Release();
            e->Release();
            host->tree = nullptr;
        });

        // Т-73: строка, построенная заново, — новый элемент (свой RuntimeId), старый элемент недоступен
        // T-73: a row built anew is a new element (its own RuntimeId), the old element is not available
        runner.add("uia grid: rebuilt row is a new element", []() {
            gran::Application app(headless());
            app.loadString(MARKUP);
            gran::AppDriver driver(app);
            driver.start();
            auto host = std::make_shared<gran::platform::win32::UiaHost>();
            host->tree = driver.window(0).state().accessTree;
            gran::platform::win32::UiaElement* top = gran::platform::win32::elementFor(host, gridRowsOf(app)[0]);
            TEST_ASSERT(top->Expand() == S_OK);
            driver.settle();
            gran::platform::win32::UiaElement* child = gran::platform::win32::elementFor(host, gridRowsOf(app)[1]);
            SAFEARRAY* before = nullptr;
            TEST_ASSERT(child->GetRuntimeId(&before) == S_OK);
            TEST_ASSERT(top->Collapse() == S_OK);
            driver.settle();
            TEST_ASSERT(top->Expand() == S_OK);
            driver.settle();
            gran::platform::win32::UiaElement* again = gran::platform::win32::elementFor(host, gridRowsOf(app)[1]);
            SAFEARRAY* after = nullptr;
            TEST_ASSERT(again->GetRuntimeId(&after) == S_OK);
            // Строка списка: {Append, список ×2, запись ×2, столбец, версия записей} — у новой копии после свёртки другая версия
            // A list row: {Append, list x2, record x2, column, record version} — the new copy after collapsing has another version
            LONG index = 6;
            LONG upper = 0;
            int first = 0;
            int second = 0;
            TEST_ASSERT(SafeArrayGetUBound(after, 1, &upper) == S_OK && upper == 6);
            SafeArrayGetElement(before, &index, &first);
            SafeArrayGetElement(after, &index, &second);
            TEST_ASSERT(first != second);
            VARIANT v;
            VariantInit(&v);
            TEST_ASSERT(child->GetPropertyValue(UIA_NamePropertyId, &v) == static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
            UiaRect rect{};
            TEST_ASSERT(child->get_BoundingRectangle(&rect) == static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
            TEST_ASSERT(again->GetPropertyValue(UIA_NamePropertyId, &v) == S_OK && v.vt == VT_BSTR);
            VariantClear(&v);
            SafeArrayDestroy(before);
            SafeArrayDestroy(after);
            again->Release();
            child->Release();
            top->Release();
            host->tree = nullptr;
        });

        // Т-73: строка, ушедшая при прокрутке, — тот же элемент «вне экрана» без ошибок; Realize возвращает её
        // T-73: a row gone on scrolling is the same "offscreen" element without errors; Realize brings it back
        runner.add("uia grid: scrolled row stays the same element", []() {
            gran::Application app(headless());
            app.loadString(R"(<Window width="400" height="300"><Resources><Data name="many"/><Selection name="ms" model="many"/></Resources>
<Stack><DataGrid id="big" items="many" selection="ms" height="150"><GridColumn field="n" width="1fr"/></DataGrid></Stack></Window>)");
            for (int number = 0; number < 300; ++number)
                app.model("many").append(gran::data::Record{{"n", std::to_string(number)}});
            gran::AppDriver driver(app);
            driver.start();
            auto host = std::make_shared<gran::platform::win32::UiaHost>();
            host->tree = driver.window(0).state().accessTree;
            gran::ui::Node* list = gran::ui::gridRows(*app.find("big"));
            gran::platform::win32::UiaElement* row = gran::platform::win32::elementFor(host, list->children[0].get());
            for (const auto& child : list->children)
            {
                if (child->kind == gran::ui::ElementKind::GRID_ROW)
                {
                    row->Release();
                    row = gran::platform::win32::elementFor(host, child.get());
                    break;
                }
            }
            SAFEARRAY* before = nullptr;
            TEST_ASSERT(row->GetRuntimeId(&before) == S_OK);
            gran::a11y::AccessRequest jump{gran::a11y::Request::REALIZE, list, std::string(), 0.0f};
            jump.index = 280;
            gran::a11y::postRequest(*host->tree, jump);
            driver.settle();
            VARIANT v;
            VariantInit(&v);
            TEST_ASSERT(row->GetPropertyValue(UIA_IsOffscreenPropertyId, &v) == S_OK && v.vt == VT_BOOL && v.boolVal == VARIANT_TRUE);
            VariantClear(&v);
            TEST_ASSERT(row->GetPropertyValue(UIA_ControlTypePropertyId, &v) == S_OK && v.lVal == UIA_DataItemControlTypeId);
            VariantClear(&v);
            // Корень фрагмента у виртуальной строки есть: без него UI Automation не соберёт RuntimeId (Т-73)
            // A virtual row has the fragment root: without it UI Automation does not build the RuntimeId (T-73)
            IRawElementProviderFragmentRoot* fragmentRoot = nullptr;
            TEST_ASSERT(row->get_FragmentRoot(&fragmentRoot) == S_OK && fragmentRoot != nullptr);
            fragmentRoot->Release();
            IRawElementProviderFragment* parent = nullptr;
            TEST_ASSERT(row->Navigate(NavigateDirection_Parent, &parent) == S_OK && parent != nullptr);
            parent->Release();
            TEST_ASSERT(row->SetFocus() == static_cast<HRESULT>(UIA_E_INVALIDOPERATION));
            IUnknown* virtualized = nullptr;
            TEST_ASSERT(row->GetPatternProvider(UIA_VirtualizedItemPatternId, &virtualized) == S_OK && virtualized != nullptr);
            virtualized->Release();
            TEST_ASSERT(row->Realize() == S_OK);
            driver.settle();
            TEST_ASSERT(row->GetPropertyValue(UIA_NamePropertyId, &v) == S_OK && v.vt == VT_BSTR && std::wstring(v.bstrVal) == L"0");
            VariantClear(&v);
            SAFEARRAY* after = nullptr;
            TEST_ASSERT(row->GetRuntimeId(&after) == S_OK);
            LONG upper = 0;
            TEST_ASSERT(SafeArrayGetUBound(after, 1, &upper) == S_OK);
            bool same = true;
            for (LONG index = 0; index <= upper; ++index)
            {
                int left = 0;
                int right = 0;
                SafeArrayGetElement(before, &index, &left);
                SafeArrayGetElement(after, &index, &right);
                same = same && left == right;
            }
            TEST_ASSERT(same);
            SafeArrayDestroy(before);
            SafeArrayDestroy(after);
            row->Release();
            host->tree = nullptr;
        });

        // Т-73 (пятая попытка): прыжок ползунком в середину, в начало и снова в ту же середину — строки середины,
        // взятые в первый раз, снова полноценные (видны в окне — значит, с ячейками)
        // T-73 (the fifth attempt): a thumb jump to the middle, to the start and to the same middle again — the middle rows
        // taken the first time are full again (shown in the window — so with cells)
        runner.add("uia grid: rows after thumb jumps", []() {
            gran::Application app(headless());
            app.loadString(R"(<Window width="400" height="300"><Resources><Data name="many"/><Selection name="ms" model="many"/></Resources>
<Stack><DataGrid id="big" items="many" selection="ms" height="150"><GridColumn field="n" width="1fr"/></DataGrid></Stack></Window>)");
            for (int number = 0; number < 300; ++number)
                app.model("many").append(gran::data::Record{{"n", std::to_string(number)}});
            gran::AppDriver driver(app);
            driver.start();
            auto host = std::make_shared<gran::platform::win32::UiaHost>();
            host->tree = driver.window(0).state().accessTree;
            gran::ui::Node* list = gran::ui::gridRows(*app.find("big"));
            gran::ui::Node& scroll = *list->parent;
            scroll.layout.scrollOffset.y = 3000.0f;
            gran::ui::markDirty(scroll, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
            driver.settle();
            std::vector<gran::platform::win32::UiaElement*> middle;
            for (const auto& child : list->children)
            {
                if (child->kind == gran::ui::ElementKind::GRID_ROW && child->visible)
                    middle.push_back(gran::platform::win32::elementFor(host, child.get()));
            }
            TEST_ASSERT(!middle.empty());
            scroll.layout.scrollOffset.y = 0.0f;
            gran::ui::markDirty(scroll, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
            driver.settle();
            scroll.layout.scrollOffset.y = 3000.0f;
            gran::ui::markDirty(scroll, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
            driver.settle();
            int full = 0;
            for (gran::platform::win32::UiaElement* row : middle)
            {
                IRawElementProviderFragment* cell = nullptr;
                if (row->Navigate(NavigateDirection_FirstChild, &cell) == S_OK && cell != nullptr)
                {
                    ++full;
                    cell->Release();
                }
                row->Release();
            }
            TEST_ASSERT(full == static_cast<int>(middle.size()));
            host->tree = nullptr;
        });

        // Ревью Т-75: ячейка строки вне видимой части — не «виртуальная строка», виртуальная строка отключённого списка
        // отключена, номер записи в RuntimeId — две половины
        // Review T-75: a cell of a row outside the visible part is not a "virtual row", a virtual row of a disabled list
        // is disabled, the record index in the RuntimeId is two halves
        runner.add("uia grid: virtual cell and disabled list", []() {
            gran::Application app(headless());
            app.loadString(R"(<Window width="400" height="300"><Resources><Data name="many"/><Selection name="ms" model="many"/></Resources>
<Stack><DataGrid id="big" items="many" selection="ms" height="150"><GridColumn field="n" width="1fr"/></DataGrid></Stack></Window>)");
            for (int number = 0; number < 300; ++number)
                app.model("many").append(gran::data::Record{{"n", std::to_string(number)}});
            gran::AppDriver driver(app);
            driver.start();
            auto host = std::make_shared<gran::platform::win32::UiaHost>();
            host->tree = driver.window(0).state().accessTree;
            gran::ui::Node* list = gran::ui::gridRows(*app.find("big"));
            gran::ui::Node* first = nullptr;
            for (const auto& child : list->children)
            {
                if (first == nullptr && child->kind == gran::ui::ElementKind::GRID_ROW && child->visible)
                    first = child.get();
            }
            TEST_ASSERT(first != nullptr);
            gran::platform::win32::UiaElement* row = gran::platform::win32::elementFor(host, first);
            gran::platform::win32::UiaElement* cell = gran::platform::win32::elementFor(host, first->children[0].get());
            SAFEARRAY* id = nullptr;
            TEST_ASSERT(row->GetRuntimeId(&id) == S_OK);
            LONG upper = 0;
            TEST_ASSERT(SafeArrayGetUBound(id, 1, &upper) == S_OK && upper == 6);
            SafeArrayDestroy(id);
            gran::a11y::AccessRequest jump{gran::a11y::Request::REALIZE, list, std::string(), 0.0f};
            jump.index = 280;
            gran::a11y::postRequest(*host->tree, jump);
            driver.settle();
            // Отключить после прокрутки: прокрутка отключённой таблицы — не предмет этого теста / Disable after scrolling: scrolling a disabled table is not this test's subject
            app.find("big")->enabled = false;
            VARIANT v;
            VariantInit(&v);
            TEST_ASSERT(row->GetPropertyValue(UIA_IsOffscreenPropertyId, &v) == S_OK && v.vt == VT_BOOL && v.boolVal == VARIANT_TRUE);
            VariantClear(&v);
            TEST_ASSERT(row->GetPropertyValue(UIA_IsEnabledPropertyId, &v) == S_OK && v.vt == VT_BOOL && v.boolVal == VARIANT_FALSE);
            VariantClear(&v);
            TEST_ASSERT(cell->GetPropertyValue(UIA_ControlTypePropertyId, &v) == static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
            IUnknown* virtualized = nullptr;
            TEST_ASSERT(cell->GetPatternProvider(UIA_VirtualizedItemPatternId, &virtualized) == S_OK && virtualized == nullptr);
            IRawElementProviderFragment* parent = nullptr;
            TEST_ASSERT(cell->Navigate(NavigateDirection_Parent, &parent) == static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
            cell->Release();
            row->Release();
            host->tree = nullptr;
        });

        runner.add("uia grid: removed node", []() {
            gran::Application app(headless());
            app.loadString(MARKUP);
            gran::AppDriver driver(app);
            driver.start();
            gran::a11y::AccessTree* tree = driver.window(0).state().accessTree;
            auto host = std::make_shared<gran::platform::win32::UiaHost>();
            host->tree = tree;
            gran::platform::win32::UiaElement* e = gran::platform::win32::elementFor(host, gridRowsOf(app)[1]);
            ISelectionItemProvider* item = nullptr;
            TEST_ASSERT(e->QueryInterface(__uuidof(ISelectionItemProvider), reinterpret_cast<void**>(&item)) == S_OK);
            app.remove(*app.find("tg"));
            driver.settle();
            BOOL selected = FALSE;
            TEST_ASSERT(item->get_IsSelected(&selected) == static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
            item->Release();
            e->Release();
            host->tree = nullptr;
        });
#else
        (void)runner;
#endif
    }
}
