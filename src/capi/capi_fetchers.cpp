#include "gran/capi/internal/bridge.hpp"

#include <utility>

namespace gran::capi
{
    ListCallbackFetcher::ListCallbackFetcher(gran_fetch_fn callback, void* user, std::string model)
        : _callback(callback), _user(user), _model(std::move(model))
    {
    }

    bool ListCallbackFetcher::hasMore() const
    {
        return more && _callback != nullptr;
    }

    // Записи язык вставляет сам: порция ядра пуста / The language inserts the records itself: the core portion is empty
    std::vector<data::Record> ListCallbackFetcher::next()
    {
        _callback(_user, _model.c_str());
        return std::vector<data::Record>();
    }

    TreeCallbackFetcher::TreeCallbackFetcher(gran_tree_fetch_fn function, void* data, std::string name)
        : callback(function), user(data), tree(std::move(name))
    {
    }

    // Узел ядра совпадает с номером ABI (корень ABI — 0, но корень не бывает ленивым) / The core node equals the ABI number (the ABI root is 0, but the root is never lazy)
    void TreeCallbackFetcher::fetchChildren(data::TreeModel&, data::NodeId node)
    {
        callback(user, tree.c_str(), node);
    }
}
