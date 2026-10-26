#include "gran/platform/win32/internal/uia_element.hpp"

namespace gran::platform::win32
{
    namespace
    {
        // Тип элемента UI Automation по роли (порядок — как a11y::Role) / The UI Automation control type by role (the order is as a11y::Role)
        long controlType(a11y::Role role) noexcept
        {
            static const long TYPES[] = {UIA_GroupControlTypeId, UIA_PaneControlTypeId, UIA_TextControlTypeId,
                UIA_ImageControlTypeId, UIA_ButtonControlTypeId, UIA_EditControlTypeId, UIA_CheckBoxControlTypeId,
                UIA_SliderControlTypeId, UIA_MenuControlTypeId, UIA_MenuItemControlTypeId};

            return TYPES[static_cast<std::size_t>(role)];
        }

        // Строка UTF-8 → BSTR; пустая строка — свойство не задано / A UTF-8 string → BSTR; an empty string — the property is not set
        bool setString(VARIANT* value, const std::string& text)
        {
            if (text.empty())
                return false;
            value->vt = VT_BSTR;
            value->bstrVal = SysAllocString(toWide(text).c_str());
            return value->bstrVal != nullptr;
        }

        // Логическое значение в виде VARIANT_BOOL / A boolean as VARIANT_BOOL
        bool setBool(VARIANT* value, bool flag) noexcept
        {
            value->vt = VT_BOOL;
            value->boolVal = VARIANT_FALSE;
            if (flag)
                value->boolVal = VARIANT_TRUE;
            return true;
        }
    }

    /*
    ** Свойства из a11y::describe. Оформительские контейнеры не входят в «представление
    ** элементов управления»: дикторы читают их детей, не останавливаясь на пустых группах.
    ** Корень — само окно: имя (заголовок) и тип Window даёт провайдер HWND, а скрыть корень
    ** нельзя — иначе его дети станут соседями других окон рабочего стола.
    **
    ** The properties from a11y::describe. Decorative containers are not in the "control view":
    ** screen readers read their children without stopping at empty groups.
    ** The root is the window itself: the name (the title) and the Window type come from the HWND provider,
    ** and the root cannot be hidden — otherwise its children would become siblings of other desktop windows.
    */
    bool nodeProperty(const UiaHost& host, const ui::Node& node, PROPERTYID property, VARIANT* value)
    {
        a11y::AccessibleInfo info = a11y::describe(node);
        bool root = &node == host.tree->root;

        if (root && (property == UIA_NamePropertyId || property == UIA_ControlTypePropertyId))
            return false;
        info.decorative = info.decorative && !root;
        switch (property)
        {
            case UIA_NamePropertyId: return setString(value, info.name);
            case UIA_HelpTextPropertyId: return setString(value, info.help);
            case UIA_AutomationIdPropertyId: return setString(value, node.id);
            case UIA_ControlTypePropertyId: value->vt = VT_I4; value->lVal = controlType(info.role); return true;
            case UIA_IsEnabledPropertyId: return setBool(value, info.enabled);
            case UIA_IsKeyboardFocusablePropertyId: return setBool(value, info.focusable && info.enabled);
            case UIA_HasKeyboardFocusPropertyId: return setBool(value, host.tree->focus == &node);
            case UIA_IsControlElementPropertyId: return setBool(value, !info.decorative);
            case UIA_IsContentElementPropertyId: return setBool(value, !info.decorative);
            case UIA_IsPasswordPropertyId: return setBool(value, node.content.password);
            default: return false;
        }
    }

    // Логические координаты окна → пиксели экрана: масштаб DPI окна и начало клиентской области
    // Logical window coordinates → screen pixels: the window DPI scale and the client area origin
    UiaRect screenRect(const UiaHost& host, const ui::Node& node) noexcept
    {
        Rect bounds = a11y::boundsOf(node);
        double scale = static_cast<double>(GetDpiForWindow(host.window)) / 96.0;
        POINT origin{0, 0};

        ClientToScreen(host.window, &origin);
        return UiaRect{origin.x + bounds.x * scale, origin.y + bounds.y * scale, bounds.width * scale,
            bounds.height * scale};
    }
}
