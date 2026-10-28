#include "gran/app/internal/app_state.hpp"

#include <utility>
#include <vector>

namespace gran::app
{
    /*
    ** Запросы клиентов доступности забираются целиком до выполнения: обработчик приложения
    ** (onClick, onChange) может снова обратиться к дереву, а провайдер — добавить запрос.
    ** Узел проверяется перед каждым запросом: предыдущий мог закрыть слой или удалить узлы.
    **
    ** The accessibility client requests are taken entirely before performing: an application handler
    ** (onClick, onChange) may access the tree again, and the provider may add a request.
    ** The node is checked before every request: the previous one may have closed a layer or removed nodes.
    */
    bool runAccessRequests(AppWindow& win)
    {
        std::vector<a11y::AccessRequest> requests;
        bool changed = false;

        if (win.access.requests.empty() || !win.router)
            return false;
        requests.swap(win.access.requests);
        for (const a11y::AccessRequest& request : requests)
        {
            if (a11y::contains(win.access, request.node))
                changed = win.router->perform(request) || changed;
        }
        return changed;
    }
}
